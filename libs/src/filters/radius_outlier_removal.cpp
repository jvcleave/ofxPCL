/*
 * Software License Agreement (BSD License)
 *
 *  Copyright (c) 2010, Willow Garage, Inc.
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above
 *     copyright notice, this list of conditions and the following
 *     disclaimer in the documentation and/or other materials provided
 *     with the distribution.
 *   * Neither the name of Willow Garage, Inc. nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 *  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 *  COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 *  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 *  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 *  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 *  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 *  ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 *
 * $Id: radius_outlier_removal.cpp 1370 2011-06-19 01:06:01Z jspricke $
 *
 */

#include "pcl/impl/instantiate.hpp"
#include "pcl/point_types.h"
#include "pcl/filters/radius_outlier_removal.h"
#include "pcl/filters/impl/radius_outlier_removal.hpp"
#include "pcl/ros/conversions.h"

//////////////////////////////////////////////////////////////////////////////////////////////
void
pcl::RadiusOutlierRemoval<sensor_msgs::PointCloud2>::applyFilter (PointCloud2 &output)
{
  output.is_dense = true;
  // If fields x/y/z are not present, we cannot filter
  if (x_idx_ == -1 || y_idx_ == -1 || z_idx_ == -1)
  {
    PCL_ERROR ("[pcl::%s::applyFilter] Input dataset doesn't have x-y-z coordinates!\n", getClassName ().c_str ());
    output.width = output.height = 0;
    output.data.clear ();
    return;
  }

  if (search_radius_ == 0.0)
  {
    PCL_ERROR ("[pcl::%s::applyFilter] No radius defined!\n", getClassName ().c_str ());
    output.width = output.height = 0;
    output.data.clear ();
    return;
  }
  // Initialize the spatial locator
  //initTree (spatial_locator_type_, tree_, k_);

  tree_.reset (new KdTreeFLANN<pcl::PointXYZ> );

  // Send the input dataset to the spatial locator
  pcl::PointCloud<pcl::PointXYZ> cloud;
  pcl::fromROSMsg (*input_, cloud);
  tree_->setInputCloud (cloud.makeShared ());

  // Allocate enough space to hold the results
  std::vector<int> nn_indices (indices_->size ());
  std::vector<float> nn_dists (indices_->size ());

  // Copy the common fields
  output.is_bigendian = input_->is_bigendian;
  output.point_step = input_->point_step;
  output.height = 1;

  output.data.resize (input_->width * input_->point_step); // reserve enough space
  removed_indices_->resize (input_->data.size ());

  int nr_p = 0;
  int nr_removed_p = 0;
  // Go over all the points and check which doesn't have enough neighbors
  for (size_t cp = 0; cp < indices_->size (); ++cp)
  {
    int k = tree_->radiusSearch ((*indices_)[cp], search_radius_, nn_indices, nn_dists);
    // Check if the number of neighbors is larger than the user imposed limit
    if (k < min_pts_radius_)
    {
      if (extract_removed_indices_)
      {
        (*removed_indices_)[nr_removed_p] = cp;
        nr_removed_p++;
      }
      continue;
    }

    memcpy (&output.data[nr_p * output.point_step], &input_->data[(*indices_)[cp] * output.point_step],
            output.point_step);
    nr_p++;
  }

  output.width = nr_p;
  output.height = 1;
  output.data.resize (output.width * output.point_step);
  output.row_step = output.point_step * output.width;

  removed_indices_->resize (nr_removed_p);
}
// Instantiations of specific point types
PCL_INSTANTIATE(RadiusOutlierRemoval, PCL_XYZ_POINT_TYPES);
