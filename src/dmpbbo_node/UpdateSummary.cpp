/**
 * @file   UpdateSummary.cpp
 * @brief  UpdateSummary class source file.
 * @author Freek Stulp
 *
 * This file is part of DmpBbo, a set of libraries and programs for the
 * black-box optimization of dynamical movement primitives.
 * Copyright (C) 2014 Freek Stulp, ENSTA-ParisTech
 *
 * DmpBbo is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * DmpBbo is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with DmpBbo.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "/home/hrcstudents/Documents/Code/catkin_ws/src/dmp_bbo_node/include/dmp_bbo_node/UpdateSummary.hpp"

#include "/home/hrcstudents/Documents/Code/catkin_ws/src/dmp_bbo_node/include/dmp_bbo_node/DistributionGaussian.hpp"
#include "/home/hrcstudents/Documents/Code/catkin_ws/src/dmp_bbo_node/include/dmp_bbo_node/EigenFileIO.hpp"

#include <iostream>
#include <iomanip>
#include <fstream>
#include <boost/filesystem.hpp>
#include <eigen3/Eigen/Core>
#include <eigen3/Eigen/Eigenvalues>

using namespace std;
using namespace Eigen;

namespace DmpBbo {

bool saveToDirectory(const UpdateSummary& summary, std::string directory, bool overwrite)
{
  // Make directory if it doesn't already exist
  if (!boost::filesystem::exists(directory))
  {
    if (!boost::filesystem::create_directories(directory))
    {
      cerr << __FILE__ << ":" << __LINE__ << ":";
      cerr << "Couldn't make directory file '" << directory << "'." << endl;
      return false;
    }
  }

  // Abbreviations to make it fit on one line
  bool ow = overwrite;
  string dir = directory;
  VectorXd cost_eval_vec = VectorXd::Constant(1,summary.cost_eval);

  if (!saveMatrix(dir, "distribution_mean.txt",  summary.distribution->mean(),  ow)) return false;
  if (!saveMatrix(dir, "distribution_covar.txt", summary.distribution->covar(), ow)) return false;
  if (!saveMatrix(dir, "cost_eval.txt",          cost_eval_vec,                 ow)) return false;
  if (!saveMatrix(dir, "samples.txt",            summary.samples,               ow)) return false;
  if (!saveMatrix(dir, "costs.txt",              summary.costs,                 ow)) return false;
  if (!saveMatrix(dir, "weights.txt",            summary.weights,               ow)) return false;
  if (!saveMatrix(dir, "distribution_new_mean.txt",  summary.distribution_new->mean(),  ow)) return false;
  if (!saveMatrix(dir, "distribution_new_covar.txt", summary.distribution_new->covar(), ow)) return false;

  //Save Hrc matrices
   if (!saveMatrixHrc(dir, "distribution_meanHrc.txt",  summary.distribution->mean(),  ow)) return false;
  if (!saveMatrixHrc(dir, "distribution_covarHrc.txt", summary.distribution->covar(), ow)) return false;
  if (!saveMatrixHrc(dir, "cost_evalHrc.txt",          cost_eval_vec,                 ow)) return false;
  if (!saveMatrixHrc(dir, "samplesHrc.txt",            summary.samples,               ow)) return false;
  if (!saveMatrixHrc(dir, "costsHrc.txt",              summary.costs,                 ow)) return false;
  if (!saveMatrixHrc(dir, "weightsHrc.txt",            summary.weights,               ow)) return false;
  if (!saveMatrixHrc(dir, "distribution_new_meanHrc.txt",  summary.distribution_new->mean(),  ow)) return false;
  if (!saveMatrixHrc(dir, "distribution_new_covarHrc.txt", summary.distribution_new->covar(), ow)) return false;

  if (summary.cost_vars_eval.size()>0)
    if (!saveMatrix(dir, "cost_vars_eval.txt",summary.cost_vars_eval, ow)) return false;
  if (summary.cost_vars.size()>0){
    if (!saveMatrix(dir, "cost_vars.txt",summary.cost_vars, ow)) return false;
    int n_samples=  summary.cost_vars.rows();
    int n_cost_vars=13;
    int n_time_steps = summary.cost_vars.cols()/n_cost_vars;
     VectorXd my_row;
    MatrixXd rollout;
    for(int k=0; k<n_samples;k++){

        std::stringstream ss;
        ss << k;
        string str = ss.str();
         std::string text="rollout"+str+".txt";
       my_row = summary.cost_vars.row(k);
        rollout = (Map<MatrixXd>(my_row.data(),n_cost_vars,n_time_steps)).transpose();
        saveMatrixHrc(dir,text,rollout, ow);
    }
  }
  return true;

}

bool saveToDirectory(const std::vector<UpdateSummary>& update_summaries, std::string directory, bool overwrite, bool only_learning_curve)
{

  // Save the learning curve
  int n_updates = update_summaries.size();
  MatrixXd learning_curve(n_updates,4);
  learning_curve(0,0) = 0; // First evaluation is at 0

  for (int i_update=0; i_update<n_updates; i_update++)
  {

    int n_samples = update_summaries[i_update].costs.rows();

    // Number of samples at which an evaluation was performed.
    if (i_update>0)
      learning_curve(i_update,0) = learning_curve(i_update-1,0) + n_samples;

    // The cost of the evaluation at this update
    learning_curve(i_update,1) = update_summaries[i_update].cost_eval;

    // The largest eigenvalue of the covariance matrix
    MatrixXd eigen_values = update_summaries[i_update].distribution->covar().eigenvalues().real();
    learning_curve(i_update,2) = sqrt(eigen_values.maxCoeff());

    double mean_costs = update_summaries[i_update].costs.sum()/n_samples;
    double std_costs = (update_summaries[i_update].costs.array() - mean_costs).abs2().sum();
    std_costs = sqrt(std_costs/(n_samples-1));
    learning_curve(i_update,3) = std_costs;


  }

  if (!saveMatrix(directory, "learning_curve.txt", learning_curve, overwrite))
    return false;

  if (!only_learning_curve)
  {
    // Save all the information in the update summaries
    for (int i_update=0; i_update<n_updates; i_update++)
    {
      stringstream stream;
      stream << directory << "/update" << setw(5) << setfill('0') << i_update+1;
      if (!saveToDirectory(update_summaries[i_update], stream.str(),overwrite))
        return false;
    }
  }
  return true;
}

bool saveToDirectoryNewUpdate(const UpdateSummary& update_summary, std::string directory)
{

  // Make directory if it doesn't already exist
  if (!boost::filesystem::exists(directory))
  {
    if (!boost::filesystem::create_directories(directory))
    {
      cerr << __FILE__ << ":" << __LINE__ << ":";
      cerr << "Couldn't make directory file '" << directory << "'." << endl;
      return false;
    }
    // Directory didn't exist yet, so this must be the first update.
    directory += "/update00001/";
    bool overwrite=true;
    return saveToDirectory(update_summary,directory,overwrite);
  }

  // Find the directory with the highest update
  // todo: this can probably be done more efficiently with boost::filesystem somehow
  int MAX_UPDATE = 99999; // Cannot store more in %05d format
  int i_update=1;
  while (i_update<MAX_UPDATE)
  {
    stringstream stream;
    stream << directory << "/update" << setw(5) << setfill('0') << i_update << "/";
    string directory_update = stream.str();
    if (!boost::filesystem::exists(directory_update))
    {
      // Found a directory that doesn't exist yet!
      bool overwrite=true;
      return saveToDirectory(update_summary,directory_update,overwrite);
    }
    i_update++;
  }

  std::cerr << "Sorry, directory " << directory << " is already full with update subdirectories." << std::endl;
  return false;
}

}
