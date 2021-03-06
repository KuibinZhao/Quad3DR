//==================================================
// sparse_stereo_matcher.hpp
//
//  Copyright (c) 2016 Benjamin Hepp.
//  Author: Benjamin Hepp
//  Created on: Sep 3, 2016
//==================================================

#include <ait/utilities.h>
#include <limits>
#include <thread>
#include <opencv2/features2d.hpp>
#if OPENCV_3
//  #include <opencv2/xfeatures2d.hpp>
//  #include <opencv2/sfm.hpp>
  #include <opencv2/core/cuda.hpp>
#else
  #include <opencv2/gpu/gpu.hpp>
  #include <opencv2/gpu/gpumat.hpp>
#endif

namespace ait
{
namespace stereo
{


#if OPENCV_2_4
  namespace cv_cuda = cv::gpu;
#else
  namespace cv_cuda = cv::cuda;
#endif


template <typename T, typename U>
FeatureDetectorOpenCV<T, U>::FeatureDetectorOpenCV(const cv::Ptr<T> &detector, const cv::Ptr<U> &descriptor_computer)
: detector_(detector),
  detector_2_(detector),
  descriptor_computer_(descriptor_computer),
  descriptor_computer_2_(descriptor_computer),
  max_num_of_keypoints_(std::numeric_limits<int>::max())
{
}

template <typename T, typename U>
FeatureDetectorOpenCV<T, U>::FeatureDetectorOpenCV(
    const cv::Ptr<T> &detector, const cv::Ptr<T> &detector_2,
    const cv::Ptr<U> &descriptor_computer, const cv::Ptr<U> &descriptor_computer_2)
: detector_(detector),
  detector_2_(detector_2),
  descriptor_computer_(descriptor_computer),
  descriptor_computer_2_(descriptor_computer_2),
  max_num_of_keypoints_(std::numeric_limits<int>::max())
{
}

template <typename T, typename U>
FeatureDetectorOpenCV<T, U>::~FeatureDetectorOpenCV()
{
}

template <typename T, typename U>
void FeatureDetectorOpenCV<T, U>::detectFeatureKeypoints(
    cv::Ptr<T> detector,
    const cv::InputArray img,
    std::vector<cv::KeyPoint> *keypoints_ptr) const
{
#if OPENCV_2_4
  detector->detect(img.getMat(), *keypoints_ptr);
#else
  detector->detect(img, *keypoints_ptr);
#endif
  if (keypoints_ptr->size() > max_num_of_keypoints_)
  {
    keypoints_ptr->resize(max_num_of_keypoints_);
  }
}

template <typename T, typename U>
void FeatureDetectorOpenCV<T, U>::computeFeatureDescriptors(
    cv::Ptr<U> descriptor_computer,
    cv::InputArray img,
    std::vector<cv::KeyPoint> *keypoints_ptr,
    cv::OutputArray descriptors) const
{
#if OPENCV_2_4
  cv::Mat descriptors_mat;
  descriptor_computer->compute(img.getMat(), *keypoints_ptr, descriptors_mat);
  descriptors.create(descriptors_mat.rows, descriptors_mat.cols, descriptors_mat.type());
  descriptors_mat.copyTo(descriptors.getMat());
#else
  descriptor_computer->compute(img.getMat(), *keypoints_ptr, descriptors);
#endif
}

template <typename T, typename U>
void FeatureDetectorOpenCV<T, U>::convertKeypointsToPoints(
    std::vector<cv::KeyPoint> *keypoints_ptr,
    std::vector<cv::Point2d> *points_ptr) const
{
  for (auto &keypoint : *keypoints_ptr)
  {
    cv::Point2d point(keypoint.pt.x, keypoint.pt.y);
    points_ptr->push_back(point);
  }
}

template <typename T, typename U>
void FeatureDetectorOpenCV<T, U>::detectFeatureKeypoints(
    const cv::InputArray img,
    std::vector<cv::KeyPoint> *keypoints_ptr) const
{
  detectFeatureKeypoints(detector_, img, keypoints_ptr);
}

template <typename T, typename U>
void FeatureDetectorOpenCV<T, U>::computeFeatureDescriptors(
    cv::InputArray img,
    std::vector<cv::KeyPoint> *keypoints_ptr,
    cv::OutputArray descriptors) const
{
  computeFeatureDescriptors(descriptor_computer_, img, keypoints_ptr, descriptors);
}

template <typename T, typename U>
void FeatureDetectorOpenCV<T, U>::detectAndComputeFeatures(
      cv::InputArray img,
      std::vector<cv::KeyPoint> *keypoints_ptr,
      cv::OutputArray descriptors,
      std::vector<cv::Point2d> *points_ptr) const
{
  ProfilingTimer timer = ProfilingTimer();
  detectFeatureKeypoints(img, keypoints_ptr);
  timer.stopAndPrintTiming("detecting keypoints");
  timer = ProfilingTimer();
  computeFeatureDescriptors(img, keypoints_ptr, descriptors);
  timer.stopAndPrintTiming("computing descriptors");
  timer = ProfilingTimer();
  convertKeypointsToPoints(keypoints_ptr, points_ptr);
  timer.stopAndPrintTiming("converting keypoints to points");
}

template <typename T, typename U>
void FeatureDetectorOpenCV<T, U>::detectAndComputeFeatures(
    cv::InputArray img_left,
    cv::InputArray img_right,
    std::vector<cv::KeyPoint> *keypoints_left_ptr,
    std::vector<cv::KeyPoint> *keypoints_right_ptr,
    cv::OutputArray descriptors_left,
    cv::OutputArray descriptors_right,
    std::vector<cv::Point2d> *points_left_ptr,
    std::vector<cv::Point2d> *points_right_ptr) const
{
  std::thread thread_left([&] ()
  {
    ProfilingTimer timer = ProfilingTimer();
    detectFeatureKeypoints(detector_, img_left, keypoints_left_ptr);
    timer.stopAndPrintTiming("detecting left keypoints");
    timer = ProfilingTimer();
    computeFeatureDescriptors(descriptor_computer_, img_left, keypoints_left_ptr, descriptors_left);
    timer.stopAndPrintTiming("computing left descriptors");
    timer = ProfilingTimer();
    convertKeypointsToPoints(keypoints_left_ptr, points_left_ptr);
    timer.stopAndPrintTiming("converting keypoints to points");
  });
  std::thread thread_right([&] ()
  {
    ProfilingTimer timer = ProfilingTimer();
    detectFeatureKeypoints(detector_2_, img_right, keypoints_right_ptr);
    timer.stopAndPrintTiming("detecting right keypoints");
    timer = ProfilingTimer();
    computeFeatureDescriptors(descriptor_computer_2_, img_right, keypoints_right_ptr, descriptors_right);
    timer.stopAndPrintTiming("computing right descriptors");
    timer = ProfilingTimer();
    convertKeypointsToPoints(keypoints_right_ptr, points_right_ptr);
    timer.stopAndPrintTiming("converting keypoints to points");
  });
  thread_left.join();
  thread_right.join();
}

template <typename T>
FeatureDetectorOpenCVSurfCuda<T>::FeatureDetectorOpenCVSurfCuda(const cv::Ptr<T> &feature_computer)
: feature_computer_(feature_computer)
{
}

template <typename T>
FeatureDetectorOpenCVSurfCuda<T>::~FeatureDetectorOpenCVSurfCuda()
{
}

template <typename T>
void FeatureDetectorOpenCVSurfCuda<T>::detectAndComputeFeatures(
      cv::InputArray img,
      std::vector<cv::KeyPoint> *keypoints_ptr,
      cv::OutputArray descriptors,
      std::vector<cv::Point2d> *points_ptr) const
{
  cv_cuda::GpuMat img_gpu;
  cv_cuda::GpuMat mask_gpu;
  cv_cuda::GpuMat keypoints_gpu;
  cv_cuda::GpuMat descriptors_gpu;
  ProfilingTimer timer = ProfilingTimer();
  cv_cuda::Stream stream;
#if OPENCV_2_4
  stream.enqueueUpload(img.getMat(), img_gpu);
#else
  img_gpu.upload(img, stream);
#endif
  timer.stopAndPrintTiming("uploading image to GPU");
  timer = ProfilingTimer();
  feature_computer_->operator()(img_gpu, mask_gpu, keypoints_gpu, stream);
  timer.stopAndPrintTiming("detecting keypoints");
  timer = ProfilingTimer();
  feature_computer_->operator()(img_gpu, mask_gpu, keypoints_gpu, descriptors_gpu, true, stream);
  timer.stopAndPrintTiming("computing descriptors");
  timer = ProfilingTimer();
  feature_computer_->downloadKeypoints(keypoints_gpu, *keypoints_ptr, stream);
#if OPENCV_2_4
  cv::Mat descriptors_mat = descriptors.create(descriptors_gpu.size(), descriptors_gpu.type());
  stream.enqueueDownload(descriptors_gpu, descriptors_mat);
#else
  descriptors_gpu.download(descriptors, stream);
#endif
  stream.waitForCompletion();
  timer.stopAndPrintTiming("downloading keypoints and descriptors from GPU");
  if (points_ptr != nullptr)
  {
    for (auto &keypoint : *keypoints_ptr)
    {
      cv::Point2d point(keypoint.pt.x, keypoint.pt.y);
      points_ptr->push_back(point);
    }
  }
}

template <typename T>
void FeatureDetectorOpenCVSurfCuda<T>::detectAndComputeFeatures(
    cv::InputArray img_left,
    cv::InputArray img_right,
    std::vector<cv::KeyPoint> *keypoints_left_ptr,
    std::vector<cv::KeyPoint> *keypoints_right_ptr,
    cv::OutputArray descriptors_left,
    cv::OutputArray descriptors_right,
    std::vector<cv::Point2d> *points_left_ptr,
    std::vector<cv::Point2d> *points_right_ptr) const
{
  cv_cuda::GpuMat img_left_gpu;
  cv_cuda::GpuMat img_right_gpu;
  cv_cuda::GpuMat mask_gpu;
  cv_cuda::GpuMat keypoints_left_gpu;
  cv_cuda::GpuMat keypoints_right_gpu;
  cv_cuda::GpuMat descriptors_left_gpu;
  cv_cuda::GpuMat descriptors_right_gpu;
  cv_cuda::Stream stream;
  std::thread thread_left([&] ()
  {
    ProfilingTimer timer = ProfilingTimer();
#if OPENCV_2_4
    stream.enqueueUpload(img_left.getMat(), img_left_gpu);
#else
    img_left_gpu.upload(img_left);
#endif
    timer.stopAndPrintTiming("uploading image to GPU");
    timer = ProfilingTimer();
    feature_computer_->operator()(img_left_gpu, mask_gpu, keypoints_left_gpu, descriptors_left_gpu, stream);
    timer.stopAndPrintTiming("detecting keypoints");
//    timer = Timer();
//    feature_computer_->operator()(img_left_gpu, mask_gpu, keypoints_left_gpu, descriptors_left_gpu, true);
//    timer.stopAndPrintTiming("computing descriptors");
    timer = ProfilingTimer();
    feature_computer_->downloadKeypoints(keypoints_left_gpu, *keypoints_left_ptr, stream);
#if OPENCV_2_4
    descriptors_left.create(descriptors_left_gpu.size(), descriptors_left_gpu.type());
    cv::Mat descriptors_left_mat = descriptors_left.getMat();
    stream.enqueueDownload(descriptors_left_gpu, descriptors_left_mat);
#else
    descriptors_left_gpu.download(descriptors_left, stream);
#endif
    timer.stopAndPrintTiming("downloading keypoints and descriptors from GPU");
    if (points_left_ptr != nullptr)
    {
      for (auto &keypoint : *keypoints_left_ptr)
      {
        cv::Point2d point(keypoint.pt.x, keypoint.pt.y);
        points_left_ptr->push_back(point);
      }
    }
  });
  std::thread thread_right([&] ()
  {
    ProfilingTimer timer = ProfilingTimer();
#if OPENCV_2_4
    stream.enqueueUpload(img_right.getMat(), img_right_gpu);
#else
    img_right_gpu.upload(img_right);
#endif
    timer.stopAndPrintTiming("uploading image to GPU");
    timer = ProfilingTimer();
    feature_computer_->operator()(img_right_gpu, mask_gpu, keypoints_right_gpu, descriptors_right_gpu);
    timer.stopAndPrintTiming("detecting keypoints");
//    timer = ProfilingTimer();
//    feature_computer_->operator()(img_right_gpu, mask_gpu, keypoints_right_gpu, descriptors_right_gpu, true);
//    timer.stopAndPrintTiming("computing descriptors");
    timer = ProfilingTimer();
    feature_computer_->downloadKeypoints(keypoints_right_gpu, *keypoints_right_ptr);
#if OPENCV_2_4
    descriptors_right.create(descriptors_right_gpu.size(), descriptors_right_gpu.type());
    cv::Mat descriptors_right_mat = descriptors_right.getMat();
    stream.enqueueDownload(descriptors_right_gpu, descriptors_right_mat);
#else
    descriptors_right_gpu.download(descriptors_right);
#endif
    timer.stopAndPrintTiming("downloading keypoints and descriptors from GPU");
    if (points_right_ptr != nullptr)
    {
      for (auto &keypoint : *keypoints_right_ptr)
      {
        cv::Point2d point(keypoint.pt.x, keypoint.pt.y);
        points_right_ptr->push_back(point);
      }
    }
  });
  thread_left.join();
  thread_right.join();
}


template <typename T>
FeatureDetectorOpenCVCuda<T>::FeatureDetectorOpenCVCuda(const cv::Ptr<T> &feature_computer)
: feature_computer_(feature_computer)
{
}

template <typename T>
FeatureDetectorOpenCVCuda<T>::~FeatureDetectorOpenCVCuda()
{
}

template <typename T>
void FeatureDetectorOpenCVCuda<T>::detectAndComputeFeatures(
      cv::InputArray img,
      std::vector<cv::KeyPoint> *keypoints_ptr,
      cv::OutputArray descriptors,
      std::vector<cv::Point2d> *points_ptr)
{
  cv_cuda::GpuMat img_gpu;
  cv_cuda::GpuMat mask_gpu;
  cv_cuda::GpuMat keypoints_gpu;
  cv_cuda::GpuMat descriptors_gpu;
  cv_cuda::Stream stream;
  ProfilingTimer timer = ProfilingTimer();
#if OPENCV_2_4
  stream.enqueueUpload(img.getMat(), img_gpu);
#else
  img_gpu.upload(img, stream);
#endif
  stream.waitForCompletion();
  timer.stopAndPrintTiming("uploading image to GPU");
  timer = ProfilingTimer();
#if OPENCV_2_4
  feature_computer_->operator()(img_gpu, mask_gpu, keypoints_gpu, descriptors_gpu);
#else
  feature_computer_->detectAndComputeAsync(img_gpu, mask_gpu, keypoints_gpu, descriptors_gpu, false, stream);
#endif
  stream.waitForCompletion();
  timer.stopAndPrintTiming("detecting keypoints");
//  timer = ProfilingTimer();
//  feature_computer_->operator()(img_gpu, mask_gpu, keypoints_gpu, descriptors_gpu, true);
//  timer.stopAndPrintTiming("computing descriptors");
  timer = ProfilingTimer();
  feature_computer_->downloadKeypoints(keypoints_gpu, *keypoints_ptr, stream);
#if OPENCV_2_4
  cv::Mat descriptors_mat = descriptors.create(descriptors_gpu.size(), descriptors_gpu.type());
  stream.enqueueDownload(descriptors_gpu, descriptors_mat);
#else
  descriptors_gpu.download(descriptors, stream);
#endif
  stream.waitForCompletion();
  timer.stopAndPrintTiming("downloading keypoints and descriptors from GPU");
  if (points_ptr != nullptr)
  {
    for (auto &keypoint : *keypoints_ptr)
    {
      cv::Point2d point(keypoint.pt.x, keypoint.pt.y);
      points_ptr->push_back(point);
    }
  }
}

template <typename T>
void FeatureDetectorOpenCVCuda<T>::detectAndComputeFeatures(
    cv::InputArray img_left,
    cv::InputArray img_right,
    std::vector<cv::KeyPoint> *keypoints_left_ptr,
    std::vector<cv::KeyPoint> *keypoints_right_ptr,
    cv::OutputArray descriptors_left,
    cv::OutputArray descriptors_right,
    std::vector<cv::Point2d> *points_left_ptr,
    std::vector<cv::Point2d> *points_right_ptr)
{
  std::cout << "FeatureDetectorOpenCVCuda" << std::endl;
  cv_cuda::GpuMat img_left_gpu;
  cv_cuda::GpuMat img_right_gpu;
  cv_cuda::GpuMat mask_left_gpu;
  cv_cuda::GpuMat mask_right_gpu;
  cv_cuda::GpuMat keypoints_left_gpu;
  cv_cuda::GpuMat keypoints_right_gpu;
  cv_cuda::GpuMat descriptors_left_gpu;
  cv_cuda::GpuMat descriptors_right_gpu;
  cv_cuda::Stream stream;
  ProfilingTimer timer = ProfilingTimer();
#if OPENCV_2_4
  stream.enqueueUpload(img_left.getMat(), img_left_gpu);
  stream.enqueueUpload(img_right.getMat(), img_right_gpu);
#else
  img_left_gpu.upload(img_left, stream);
  img_right_gpu.upload(img_right, stream);
#endif
  timer.stopAndPrintTiming("uploading images to GPU");
  timer = ProfilingTimer();
#if OPENCV_2_4
  feature_computer_->operator()(img_left_gpu, mask_left_gpu, keypoints_left_gpu, descriptors_left_gpu);
  feature_computer_->operator()(img_right_gpu, mask_right_gpu, keypoints_right_gpu, descriptors_right_gpu);
#else
  feature_computer_->detectAndComputeAsync(img_left_gpu, mask_left_gpu, keypoints_left_gpu, descriptors_left_gpu, false, stream);
  feature_computer_->detectAndComputeAsync(img_right_gpu, mask_right_gpu, keypoints_right_gpu, descriptors_right_gpu, false, stream);
#endif
  stream.waitForCompletion();
  timer.stopAndPrintTiming("detecting keypoints");
  timer = ProfilingTimer();
#if OPENCV_2_4
  stream.waitForCompletion();
  // Download left keypoints and descriptors
  cv::Mat keypoints_left_mat(keypoints_left_gpu.rows, keypoints_left_gpu.cols, keypoints_left_gpu.type());
  stream.enqueueDownload(keypoints_left_gpu, keypoints_left_mat);
  descriptors_left.create(descriptors_left_gpu.size(), descriptors_left_gpu.type());
  cv::Mat descriptors_left_mat = descriptors_left.getMat();
  stream.enqueueDownload(descriptors_left_gpu, descriptors_left_mat);
  // Download right keypoints and descriptors
  cv::Mat keypoints_right_mat(keypoints_right_gpu.rows, keypoints_right_gpu.cols, keypoints_right_gpu.type());
  stream.enqueueDownload(keypoints_right_gpu, keypoints_right_mat);
  descriptors_right.create(descriptors_right_gpu.size(), descriptors_right_gpu.type());
  cv::Mat descriptors_right_mat = descriptors_right.getMat();
  stream.enqueueDownload(descriptors_right_gpu, descriptors_right_mat);
#else
  cv::Mat keypoints_left_mat;
  keypoints_left_gpu.download(keypoints_left_mat, stream);
  descriptors_left_gpu.download(descriptors_left, stream);
  cv::Mat keypoints_right_mat;
  keypoints_right_gpu.download(keypoints_right_mat, stream);
  descriptors_right_gpu.download(descriptors_right, stream);
#endif
  stream.waitForCompletion();
  timer.stopAndPrintTiming("downloading left keypoints and descriptors from GPU");
  timer = ProfilingTimer();
#if OPENCV_2_4
  feature_computer_->convertKeyPoints(keypoints_left_mat, *keypoints_left_ptr);
  feature_computer_->convertKeyPoints(keypoints_right_mat, *keypoints_right_ptr);
#else
  feature_computer_->convert(keypoints_left_mat, *keypoints_left_ptr);
  feature_computer_->convert(keypoints_right_mat, *keypoints_right_ptr);
#endif
  if (points_left_ptr != nullptr)
  {
    for (auto &keypoint : *keypoints_left_ptr)
    {
      cv::Point2d point(keypoint.pt.x, keypoint.pt.y);
      points_left_ptr->push_back(point);
    }
  }
  if (points_right_ptr != nullptr)
  {
    for (auto &keypoint : *keypoints_right_ptr)
    {
      cv::Point2d point(keypoint.pt.x, keypoint.pt.y);
      points_right_ptr->push_back(point);
    }
  }
  timer.stopAndPrintTiming("converting keypoints and points from GPU");
}


template <typename T>
SparseStereoMatcher<T>::Error::Error(const std::string &msg)
: std::runtime_error(msg)
{
}

template <typename T>
SparseStereoMatcher<T>::Error::~Error()
{
}

template <typename T>
SparseStereoMatcher<T>::SparseStereoMatcher(const cv::Ptr<T> &feature_detector, const StereoCameraCalibration &calib)
: feature_detector_(feature_detector),
  calib_(calib),
  ratio_test_threshold_(0.7),
  epipolar_constraint_threshold_(0.5),
  match_norm_(cv::NORM_L2),
  bf_matcher_cross_check_(true),
  flann_index_params_(cv::makePtr<cv::flann::KDTreeIndexParams>(5)),
  flann_search_params_(cv::makePtr<cv::flann::SearchParams>(50, 0, true))
{
}

template <typename T>
SparseStereoMatcher<T>::~SparseStereoMatcher()
{
}


template <typename T>
cv::Mat SparseStereoMatcher<T>::undistortPoints(cv::InputArray points, cv::InputArray camera_matrix, cv::InputArray dist_coefficients) const
{
  cv::InputArray R = cv::noArray();
  cv::InputArray P = camera_matrix;
  cv::Mat undistorted_points;
  cv::undistortPoints(points, undistorted_points, camera_matrix, dist_coefficients, R, P);
  return undistorted_points;
}

template <typename T>
void SparseStereoMatcher<T>::undistortPoints(cv::InputOutputArray left_points, cv::InputOutputArray right_points) const
{
  left_points.getMat() = undistortPoints(left_points, calib_.left.camera_matrix, calib_.left.dist_coeffs);
  right_points.getMat() = undistortPoints(right_points, calib_.right.camera_matrix, calib_.right.dist_coeffs);
}

template <typename T>
std::vector<cv::DMatch> SparseStereoMatcher<T>::filterMatchesWithMinimumDisparity(
    const std::vector<cv::Point2d> &left_points, const std::vector<cv::Point2d> &right_points,
    const std::vector<cv::DMatch> &matches,
    double min_disparity,
    bool verbose) const
{
  std::vector<cv::DMatch> filtered_matches;
  for (int i = 0; i < matches.size(); i++)
  {
    double left_x = left_points[matches[i].queryIdx].x;
    double right_x = right_points[matches[i].trainIdx].x;
    double disparity = left_x - right_x;
    if (disparity >= min_disparity)
    {
      filtered_matches.push_back(matches[i]);
    }
  }
  if (verbose)
  {
    std::cout << "Keeping " << filtered_matches.size() << " out of " << matches.size() << " matches based on minimum disparity" << std::endl;
  }
  return filtered_matches;
}

template <typename T>
std::vector<cv::DMatch> SparseStereoMatcher<T>::filterMatchesWithMaximumDisparity(
    const std::vector<cv::Point2d> &left_points, const std::vector<cv::Point2d> &right_points,
    const std::vector<cv::DMatch> &matches,
    double max_disparity,
    bool verbose) const
{
  std::vector<cv::DMatch> filtered_matches;
  for (int i = 0; i < matches.size(); i++)
  {
    double left_x = left_points[matches[i].queryIdx].x;
    double right_x = right_points[matches[i].trainIdx].x;
    double disparity = left_x - right_x;
    if (disparity <= max_disparity)
    {
      filtered_matches.push_back(matches[i]);
    }
  }
  if (verbose)
  {
    std::cout << "Keeping " << filtered_matches.size() << " out of " << matches.size() << " matches based on maximum disparity" << std::endl;
  }
  return filtered_matches;
}

template <typename T>
std::vector<cv::DMatch> SparseStereoMatcher<T>::filterMatchesWithDistance(const std::vector<cv::DMatch> &all_matches, double good_threshold_multiplier, double min_good_threshold) const
{
  double max_dist = -std::numeric_limits<double>::infinity();
  double min_dist = std::numeric_limits<double>::infinity();
  // Quick calculation of max and min distances between keypoints
  for (int i = 0; i < all_matches.size(); i++)
  {
    double dist = all_matches[i].distance;
    if (dist < min_dist)
    {
      min_dist = dist;
    }
    if (dist > max_dist)
    {
      max_dist = dist;
    }
  }
//  std::cout << "-- Max dist: " << max_dist << ", Min dist: " << min_dist << std::endl;
  // Keep only "good" matches (i.e. whose distance is less than 2*min_dist,
  // or a small arbitary value in case that min_dist is very small).
  std::vector<cv::DMatch> good_matches;
  double good_threshold = std::max(good_threshold_multiplier * min_dist, min_good_threshold);
  for (int i = 0; i < all_matches.size(); i++)
  {
    if (all_matches[i].distance <= good_threshold)
    {
      good_matches.push_back(all_matches[i]);
    }
  }
  return good_matches;
}

template <typename T>
std::vector<cv::DMatch> SparseStereoMatcher<T>::filterMatchesWithLoweRatioTest(
    const std::vector<std::vector<cv::DMatch>> &matches,
    double ratio_test_threshold,
    bool verbose) const
{
  if (ratio_test_threshold < 0)
  {
    ratio_test_threshold = ratio_test_threshold_;
  }
  std::vector<cv::DMatch> filtered_matches;
  ProfilingTimer timer = ProfilingTimer();
  if (ratio_test_threshold >= 1.0)
  {
    // Lowe's ratio test
    for (auto knn_matches : matches)
    {
      if (knn_matches.size() > 0)
      {
        filtered_matches.push_back(knn_matches[0]);
      }
    }
  }
  else
  {
    // Lowe's ratio test
    for (auto knn_matches : matches)
    {
      if (knn_matches.size() >= 2)
      {
        if (knn_matches[0].distance < ratio_test_threshold * knn_matches[1].distance)
        {
          filtered_matches.push_back(knn_matches[0]);
        }
      }
    }
  }
  if (verbose)
  {
    std::cout << "Keeping " << filtered_matches.size() << " out of " << matches.size() << " matches based on Lowe's test" << std::endl;
  }
  timer.stopAndPrintTiming("Lowe's ratio test filtering");
  return filtered_matches;
}

template <typename T>
std::vector<cv::DMatch> SparseStereoMatcher<T>::matchFeaturesBf(cv::InputArray left_descriptors, cv::InputArray right_descriptors) const
{
//   Brute-Force matching
  cv::BFMatcher matcher(match_norm_, bf_matcher_cross_check_);
  std::vector<cv::DMatch> matches;
#if OPENCV_2_4
  matcher.match(left_descriptors.getMat(), right_descriptors.getMat(), matches);
#else
  matcher.match(left_descriptors, right_descriptors, matches);
#endif
  return matches;
}

template <typename T>
std::vector<cv::DMatch> SparseStereoMatcher<T>::matchFeaturesCustom(
    const std::vector<cv::Point2d> &left_points, const std::vector<cv::Point2d> &right_points,
    cv::InputArray left_descriptors, cv::InputArray right_descriptors, bool verbose) const
{
  cv::Mat left_descriptors_mat = left_descriptors.getMat();
  if (left_descriptors_mat.type() == CV_64F)
  {
    left_descriptors_mat.convertTo(left_descriptors_mat, CV_32F);
  }
  cv::Mat right_descriptors_mat = right_descriptors.getMat();
  if (right_descriptors_mat.type() == CV_64F)
  {
    right_descriptors_mat.convertTo(right_descriptors_mat, CV_32F);
  }

  ProfilingTimer timer;
  std::vector<int> left_y_sorted_indices;
  std::vector<int> right_y_sorted_indices;
  left_y_sorted_indices.reserve(left_descriptors_mat.rows);
  right_y_sorted_indices.reserve(right_descriptors_mat.rows);
  for (int i = 0; i < left_descriptors_mat.rows; ++i)
  {
    left_y_sorted_indices.push_back(i);
  }
  for (int i = 0; i < right_descriptors_mat.rows; ++i)
  {
    right_y_sorted_indices.push_back(i);
  }
  std::sort(left_y_sorted_indices.begin(), left_y_sorted_indices.end(), [&](int a, int b)
  {
    return left_points[a].y < left_points[b].y;
  });
  std::sort(right_y_sorted_indices.begin(), right_y_sorted_indices.end(), [&](int a, int b)
  {
    return right_points[a].y < right_points[b].y;
  });
  timer.stopAndPrintTiming("Sorting keypoint indices by y");

  timer.start();
  double max_y_diff = 10;
  std::vector<cv::DMatch> matches;
  int right_begin_j = 0;
  int right_end_j = 0;
  for (int i = 0; i < left_points.size(); ++i)
  {
    double best_score = std::numeric_limits<double>::infinity();
    int best_index = -1;
    while (right_end_j < right_points.size() - 1
        && right_points[right_end_j].y < left_points[i].y + max_y_diff)
    {
      ++right_end_j;
    }
    while (right_begin_j < right_end_j
        && right_points[right_begin_j].y - max_y_diff > left_points[i].y)
    {
      ++right_begin_j;
    }
    for (int j = right_begin_j; j <= right_end_j; ++j)
    {
      double score = cv::norm(left_descriptors_mat.row(i), right_descriptors_mat.row(j), cv::NORM_L2);
      if (score < best_score)
      {
        best_score = score;
        best_index = j;
      }
    }
    if (best_index >= 0)
    {
      matches.push_back(cv::DMatch(i, best_index, best_score));
    }
  }
  timer.stopAndPrintTiming("performing epipolar-based matching");
  return matches;
//  std::vector<cv::DMatch> filtered_matches = filterMatchesWithLoweRatioTest(all_matches, ratio_test_threshold, verbose);
//  return filtered_matches;
}

template <typename T>
std::vector<cv::DMatch> SparseStereoMatcher<T>::matchFeaturesBfKnn2(cv::InputArray left_descriptors, cv::InputArray right_descriptors, double ratio_test_threshold, bool verbose) const
{
  cv::Mat left_descriptors_mat = left_descriptors.getMat();
  if (left_descriptors_mat.type() == CV_64F)
  {
    left_descriptors_mat.convertTo(left_descriptors_mat, CV_32F);
  }
  cv::Mat right_descriptors_mat = right_descriptors.getMat();
  if (right_descriptors_mat.type() == CV_64F)
  {
    right_descriptors_mat.convertTo(right_descriptors_mat, CV_32F);
  }

  ProfilingTimer timer = ProfilingTimer();
  // Brute-Force matching with kNN (k=2)
  int k = 2;
  cv::BFMatcher matcher(match_norm_);
  timer.stopAndPrintTiming("creating brute-force knn2 matcher");
  std::vector<std::vector<cv::DMatch>> all_matches;
  timer = ProfilingTimer();
  matcher.knnMatch(left_descriptors_mat, right_descriptors_mat, all_matches, k);
  timer.stopAndPrintTiming("performing brute-force knn2 matching");
  std::vector<cv::DMatch> filtered_matches = filterMatchesWithLoweRatioTest(all_matches, ratio_test_threshold, verbose);
  return filtered_matches;
}

template <typename T>
std::vector<cv::DMatch> SparseStereoMatcher<T>::matchFeaturesFlann(cv::InputArray left_descriptors, cv::InputArray right_descriptors) const
{
  // FLANN matching
  cv::FlannBasedMatcher matcher(flann_index_params_, flann_search_params_);
  std::vector<cv::DMatch> all_matches;
#if OPENCV_2_4
  matcher.match(left_descriptors.getMat(), right_descriptors.getMat(), all_matches);
#else
  matcher.match(left_descriptors, right_descriptors, all_matches);
#endif
  return filterMatchesWithDistance(all_matches);
}

template <typename T>
std::vector<cv::DMatch> SparseStereoMatcher<T>::matchFeaturesFlannKnn2(cv::InputArray left_descriptors, cv::InputArray right_descriptors, double ratio_test_threshold, bool verbose) const
{
  cv::Mat left_descriptors_mat = left_descriptors.getMat();
  if (left_descriptors_mat.type() == CV_64F)
  {
    left_descriptors_mat.convertTo(left_descriptors_mat, CV_32F);
  }
  cv::Mat right_descriptors_mat = right_descriptors.getMat();
  if (right_descriptors_mat.type() == CV_64F)
  {
    right_descriptors_mat.convertTo(right_descriptors_mat, CV_32F);
  }

  ProfilingTimer timer = ProfilingTimer();
  // FLANN matching with kNN (k=2)
  int k = 2;
  cv::FlannBasedMatcher matcher(flann_index_params_, flann_search_params_);
  timer.stopAndPrintTiming("creating flann matcher");
  std::vector<std::vector<cv::DMatch>> all_matches;
  timer = ProfilingTimer();
  matcher.knnMatch(left_descriptors_mat, right_descriptors_mat, all_matches, k);
  timer.stopAndPrintTiming("flann knn2 matching");
  std::vector<cv::DMatch> filtered_matches = filterMatchesWithLoweRatioTest(all_matches, ratio_test_threshold, verbose);
  return filtered_matches;
}

template <typename T>
template <typename V>
std::vector<cv::Point3_<V>> SparseStereoMatcher<T>::triangulatePoints(const std::vector<cv::Point_<V>> &left_points, const std::vector<cv::Point_<V>> &right_points) const
{
  CV_Assert(left_points.size() == right_points.size());
  int num_of_points = left_points.size();
  cv::Mat points_4d;
  cv::triangulatePoints(calib_.projection_matrix_left, calib_.projection_matrix_right, left_points, right_points, points_4d);
  CV_Assert(points_4d.rows == 4);

  cv::Mat points_3d(3, num_of_points, points_4d.type());
  for (int i = 0; i < num_of_points; ++i)
  {
    V w = points_4d.at<V>(3, i);
    if (w != 0)
    {
      points_3d.at<V>(0, i) = points_4d.at<V>(0, i) / w;
      points_3d.at<V>(1, i) = points_4d.at<V>(1, i) / w;
      points_3d.at<V>(2, i) = points_4d.at<V>(2, i) / w;
    }
    else
    {
      points_3d.at<V>(0, i) = 0;
      points_3d.at<V>(1, i) = 0;
      points_3d.at<V>(2, i) = 0;
    }
  }

  std::vector<cv::Point3_<V>> points_3d_vec(num_of_points);
  // For debugging
//  std::cout << "-- 3d points --" << std::endl;
  for (int i = 0; i < num_of_points; ++i)
  {
    points_3d_vec[i] = cv::Point3_<V>(points_3d.col(i));
    // For debugging
//    std::cout << "-- " << i << " --" << std::endl;
//    std::cout << "left = " << left_points[i] << std::endl;
//    std::cout << "right = " << right_points[i] << std::endl;
//    V dx = right_points[i].x - left_points[i].x;
//    V dy = right_points[i].y - left_points[i].y;
//    std::cout << "dx = " << dx << std::endl;
//    std::cout << "dy = " << dy << std::endl;
//    std::cout << "X = " << points_3d_vec[i] << std::endl;
  }
  return points_3d_vec;
}

template <typename T>
std::vector<cv::DMatch> SparseStereoMatcher<T>::filterMatchesWithEpipolarConstraint(
    const std::vector<cv::DMatch> &matches,
    const cv::Mat &left_undist_points, const cv::Mat &right_undist_points,
    std::vector<double> *best_epipolar_constraints,
    bool verbose) const
{
  if (best_epipolar_constraints != nullptr)
  {
    best_epipolar_constraints->clear();
  }

  // For debugging
//  std::cout << "-- epipolar constraints --" << std::endl;
  // Compute epipolar constraint of matches
  std::vector<cv::DMatch> best_matches;
  for (int i = 0; i < matches.size(); ++i)
  {
    cv::Mat point1 = cv::Mat(1, 1, CV_64F);
    point1.at<cv::Point2d>(0, 0) = left_undist_points.at<cv::Point2d>(matches[i].queryIdx);
    cv::Mat point2 = cv::Mat(1, 1, CV_64F);
    point2.at<cv::Point2d>(0, 0) = right_undist_points.at<cv::Point2d>(matches[i].trainIdx);
    double epipolar_constraint = computeEpipolarConstraint(point1, point2);

    if (std::abs(epipolar_constraint) < epipolar_constraint_threshold_)
    {
      best_matches.push_back(matches[i]);
      if (best_epipolar_constraints != nullptr)
      {
        best_epipolar_constraints->push_back(epipolar_constraint);
      }
    }
    // For debugging
//    std::cout << i << ": " << epipolar_constraint << std::endl;
  }

  if (verbose)
  {
    std::cout << "Keeping " << best_matches.size() << " from " << matches.size() << " based on epipolar constraint" << std::endl;
  }

  // For debugging
//  if (best_epipolar_constraints != nullptr)
//  {
//    std::cout << best_epipolar_constraints->size() << std::endl;
//    for (int i = 0; i < best_epipolar_constraints->size(); ++i)
//    {
//      double epipolar_constraint = (*best_epipolar_constraints)[i];
//      std::cout << i << ": " << epipolar_constraint << std::endl;
//    }
//  }

  return best_matches;
}

template <typename T>
std::vector<cv::DMatch> SparseStereoMatcher<T>::filterMatchesWithEpipolarConstraint(
    const std::vector<cv::DMatch> &matches,
    const std::vector<cv::Point2d> &left_undist_points, const std::vector<cv::Point2d> &right_undist_points,
    std::vector<double> *best_epipolar_constraints,
    bool verbose) const
{
  if (best_epipolar_constraints != nullptr)
  {
    best_epipolar_constraints->clear();
  }

  // For debugging
//  std::cout << "-- epipolar constraints --" << std::endl;
  // Compute epipolar constraint of matches
  std::vector<cv::DMatch> best_matches;
  for (int i = 0; i < matches.size(); ++i)
  {
    cv::Mat point1 = cv::Mat(1, 2, CV_64F);
    point1.at<cv::Point2d>(0, 0) = left_undist_points[matches[i].queryIdx];
    cv::Mat point2 = cv::Mat(1, 2, CV_64F);
    point2.at<cv::Point2d>(0, 0) = right_undist_points[matches[i].trainIdx];
    double epipolar_constraint = computeEpipolarConstraint(point1, point2);

    if (std::abs(epipolar_constraint) < epipolar_constraint_threshold_)
    {
      best_matches.push_back(matches[i]);
      if (best_epipolar_constraints != nullptr)
      {
        best_epipolar_constraints->push_back(epipolar_constraint);
      }
    }
    // For debugging
//    std::cout << i << ": " << epipolar_constraint << std::endl;
  }

  if (verbose)
  {
    std::cout << "Keeping " << best_matches.size() << " from " << matches.size() << " based on epipolar constraint" << std::endl;
  }

  // For debugging
//  if (best_epipolar_constraints != nullptr)
//  {
//    std::cout << best_epipolar_constraints->size() << std::endl;
//    for (int i = 0; i < best_epipolar_constraints->size(); ++i)
//    {
//      double epipolar_constraint = (*best_epipolar_constraints)[i];
//      std::cout << i << ": " << epipolar_constraint << std::endl;
//    }
//  }

  return best_matches;
}

template <typename T>
void SparseStereoMatcher<T>::retrieveMatchedPointsAndUpdateMatches(
    const std::vector<cv::Point2d> &left_points, const std::vector<cv::Point2d> &right_points,
    std::vector<cv::DMatch> *matches,
    std::vector<cv::Point2d> *left_match_points, std::vector<cv::Point2d> *right_match_points) const
{
  if (left_match_points != nullptr)
  {
    left_match_points->clear();
  }
  if (right_match_points != nullptr)
  {
    right_match_points->clear();
  }

  for (int i = 0; i < matches->size(); ++i)
  {
    const cv::Point2d &left_point = left_points[(*matches)[i].queryIdx];
    const cv::Point2d &right_point = right_points[(*matches)[i].trainIdx];
    if (left_match_points != nullptr)
    {
      left_match_points->push_back(left_point);
    }
    if (right_match_points != nullptr)
    {
      right_match_points->push_back(right_point);
    }
    (*matches)[i].queryIdx = i;
    (*matches)[i].trainIdx = i;
  }
}

template <typename T>
void SparseStereoMatcher<T>::retrieveMatchedPointsAndUpdateMatches(
    const std::vector<cv::KeyPoint> &left_keypoints, const std::vector<cv::KeyPoint> &right_keypoints,
    std::vector<cv::DMatch> *matches,
    std::vector<cv::Point2d> *left_match_points, std::vector<cv::Point2d> *right_match_points,
    std::vector<cv::KeyPoint> *left_match_keypoints, std::vector<cv::KeyPoint> *right_match_keypoints) const
{
  if (left_match_points != nullptr)
  {
    left_match_points->clear();
  }
  if (right_match_points != nullptr)
  {
    right_match_points->clear();
  }
  if (left_match_keypoints != nullptr)
  {
    left_match_keypoints->clear();
  }
  if (right_match_keypoints != nullptr)
  {
    right_match_keypoints->clear();
  }

  for (int i = 0; i < matches->size(); ++i)
  {
    const cv::KeyPoint &left_keypoint = left_keypoints[(*matches)[i].queryIdx];
    const cv::KeyPoint &right_keypoint = right_keypoints[(*matches)[i].trainIdx];
    const cv::Point2f &left_point = left_keypoints[(*matches)[i].queryIdx].pt;
    const cv::Point2f &right_point = right_keypoints[(*matches)[i].trainIdx].pt;
    if (left_match_points != nullptr)
    {
      left_match_points->push_back(cv::Point2d(left_point.x, left_point.y));
    }
    if (right_match_points != nullptr)
    {
      right_match_points->push_back(cv::Point2d(right_point.x, right_point.y));
    }
    if (left_match_keypoints != nullptr)
    {
      left_match_keypoints->push_back(left_keypoint);
      left_match_keypoints->back().pt = left_point;
    }
    if (right_match_keypoints != nullptr)
    {
      right_match_keypoints->push_back(right_keypoint);
      right_match_keypoints->back().pt = right_point;
    }
    (*matches)[i].queryIdx = i;
    (*matches)[i].trainIdx = i;
  }
}

template <typename T>
void SparseStereoMatcher<T>::correctMatches(
    std::vector<cv::Point2d> *left_points, std::vector<cv::Point2d> *right_points) const
{
  CV_Assert(left_points->size() == right_points->size());
  std::vector<cv::Point2d> left_correct_points;
  std::vector<cv::Point2d> right_correct_points;
  ProfilingTimer timer = ProfilingTimer();
  cv::correctMatches(calib_.fundamental_matrix, *left_points, *right_points, left_correct_points, right_correct_points);
  timer.stopAndPrintTiming("cv::correctMatches");
  timer = ProfilingTimer();
  *left_points = std::move(left_correct_points);
  *right_points = std::move(right_correct_points);
  timer.stopAndPrintTiming("moving corrected matches");
}

template <typename T>
void SparseStereoMatcher<T>::correctMatchesAndUpdateKeypoints(
    std::vector<cv::Point2d> *left_points, std::vector<cv::Point2d> *right_points,
    std::vector<cv::KeyPoint> *left_keypoints, std::vector<cv::KeyPoint> *right_keypoints) const
{
  CV_Assert(left_keypoints->size() == right_keypoints->size());
  CV_Assert(left_keypoints->size() == left_points->size());
  correctMatches(left_points, right_points);
  // Update keypoints with corrected points
  for (int i = 0; i < left_points->size(); ++i)
  {
    const cv::Point2d &left_correct_point = (*left_points)[i];
    (*left_keypoints)[i].pt = cv::Point2f(left_correct_point.x, left_correct_point.y);
    const cv::Point2d &right_correct_point = (*right_points)[i];
    (*right_keypoints)[i].pt = cv::Point2f(right_correct_point.x, right_correct_point.y);
  }
}

template <typename T>
double SparseStereoMatcher<T>::computeEpipolarConstraint(const cv::Mat &point1, const cv::Mat &point2) const
{
  CV_Assert(point1.type() == point2.type());
  if (point1.type() == CV_64FC2)
  {
    cv::Mat point1_flat(point1.rows, 2, CV_64FC1);
    point1_flat.at<cv::Point2d>(0, 0) = point1.at<cv::Point2d>(0, 0);
    cv::Mat point2_flat(point1.rows, 2, CV_64FC1);
    point2_flat.at<cv::Point2d>(0, 0) = point2.at<cv::Point2d>(0, 0);
    return computeEpipolarConstraint(point1_flat, point2_flat);
  }
  else if (point1.type() == CV_32FC2)
  {
    cv::Mat point1_flat(point1.rows, 2, CV_32FC1);
    point1_flat.at<cv::Point2f>(0, 0) = point1.at<cv::Point2f>(0, 0);
    cv::Mat point2_flat(point1.rows, 2, CV_32FC1);
    point2_flat.at<cv::Point2f>(0, 0) = point2.at<cv::Point2f>(0, 0);
    return computeEpipolarConstraint(point1_flat, point2_flat);
  }
  else
  {
    cv::Mat hom_point1;
    Utilities::convertPointsToHomogeneous(point1, hom_point1);
    cv::Mat hom_point2;
    Utilities::convertPointsToHomogeneous(point2, hom_point2);
    // For debugging
//    std::cout << "fundamental_matrix: " << calib_.fundamental_matrix << std::endl;
//    std::cout << "hom_point1: " << hom_point1 << std::endl;
//    std::cout << "hom_point2: " << hom_point2 << std::endl;
    cv::Mat epipolar_constraint_mat = hom_point2 * calib_.fundamental_matrix * hom_point1.t();
    CV_Assert(epipolar_constraint_mat.rows == 1 && epipolar_constraint_mat.cols == 1);

    double epipolar_constraint;
    if (epipolar_constraint_mat.type() == CV_32F)
    {
      epipolar_constraint = static_cast<double>(epipolar_constraint_mat.at<float>(0, 0));
    }
    else if (epipolar_constraint_mat.type() == CV_64F)
    {
      epipolar_constraint = epipolar_constraint_mat.at<double>(0, 0);
    }
    else
    {
      throw std::runtime_error("Cannot compute epipolar constraint for non-floating point matrices");
    }
    return epipolar_constraint;
  }
}

template <typename T>
double SparseStereoMatcher<T>::computeEpipolarConstraint(const cv::Point2d &point1, const cv::Point2d &point2) const
{
  cv::Mat point1_mat(1, 1, CV_64FC2);
  point1_mat.at<cv::Point2d>(0, 0) = point1;
  cv::Mat point2_mat(1, 1, CV_64FC2);
  point2_mat.at<cv::Point2d>(0, 0) = point2;
  return computeEpipolarConstraint(point1_mat, point2_mat);
}

template <typename T>
double SparseStereoMatcher<T>::computeEpipolarConstraint(const cv::Point2f &point1, const cv::Point2f &point2) const
{
  cv::Mat point1_mat(1, 1, CV_32FC2);
  point1_mat.at<cv::Point2d>(0, 0) = point1;
  cv::Mat point2_mat(1, 1, CV_32FC2);
  point2_mat.at<cv::Point2d>(0, 0) = point2;
  return computeEpipolarConstraint(point1_mat, point2_mat);
}

template <typename T>
std::vector<double> SparseStereoMatcher<T>::computeEpipolarConstraints(
    const std::vector<cv::Point2d> &left_points, const std::vector<cv::Point2d> &right_points) const
{
  CV_Assert(left_points.size() == right_points.size());
  std::vector<double> epipolar_constraints(left_points.size());
  // For debugging
//  std::cout << "-- epipolar constraints --" << std::endl;
  for (int i = 0; i < left_points.size(); ++i)
  {
    epipolar_constraints[i] = computeEpipolarConstraint(left_points[i], right_points[i]);
  }
  return epipolar_constraints;
}

template <typename T>
std::vector<cv::Point3d> SparseStereoMatcher<T>::match(
    const cv::InputArray left_input_img, cv::InputArray right_input_img,
    std::vector<cv::Point2d> *left_image_points,
    std::vector<cv::Point2d> *right_image_points,
    bool verbose)
{
  CV_Assert(left_input_img.channels() == 1);
  CV_Assert(right_input_img.channels() == 1);
  cv::Mat left_img = left_input_img.getMat();
  cv::Mat right_img = right_input_img.getMat();

  // Compute feature points and match them
  ProfilingTimer timer = ProfilingTimer();
  std::vector<cv::KeyPoint> left_keypoints;
  std::vector<cv::KeyPoint> right_keypoints;
  cv::Mat left_descriptors;
  cv::Mat right_descriptors;
  std::vector<cv::Point2d> left_points;
  std::vector<cv::Point2d> right_points;
//  feature_detector_->detectAndComputeFeatures(left_img, &left_keypoints, left_descriptors, &left_points);
//  feature_detector_->detectAndComputeFeatures(right_img, &right_keypoints, right_descriptors, &right_points);
  feature_detector_->detectAndComputeFeatures(
      left_img, right_img,
      &left_keypoints, &right_keypoints,
      left_descriptors, right_descriptors,
      &left_points, &right_points);
  if (verbose)
  {
    std::cout << "Detected " << left_points.size() << " left and " << right_points.size() << " right keypoints" << std::endl;
  }
  timer.stopAndPrintTiming("detecting and computing features");

  if (left_points.size() == 0 || right_points.size() == 0)
  {
    throw Error("Unable to detect any keypoints");
  }

  // For debugging
//  timer = ProfilingTimer();
//  auto left_img_with_keypoints = ait::Utilities::drawPoints(left_input_img.getMat().clone(), left_points);
//  auto right_img_with_keypoints = ait::Utilities::drawPoints(right_input_img.getMat().clone(), right_points);
//  cv::imshow("Left keypoints", left_img_with_keypoints);
//  cv::imshow("Right keypoints", right_img_with_keypoints);
//  timer.stopAndPrintTiming("drawing keypoints");

  timer = ProfilingTimer();
//  std::vector<cv::DMatch> matches = matchFeaturesBf(left_descriptors, right_descriptors);
//  std::vector<cv::DMatch> matches = matchFeaturesBfKnn2(left_descriptors, right_descriptors, -1.0, verbose);
//  std::vector<cv::DMatch> matches = matchFeaturesFlannKnn2(left_descriptors, right_descriptors, -1.0, verbose);
  std::vector<cv::DMatch> matches = matchFeaturesCustom(left_points, right_points, left_descriptors, right_descriptors, verbose);
  timer.stopAndPrintTiming("feature matching");

  if (matches.size() == 0)
  {
    throw Error("Unable to match any keypoints");
  }
  // TODO (add parameter)
  double min_disparity = 10;
  matches = filterMatchesWithMinimumDisparity(left_points, right_points, matches, min_disparity, verbose);

  // For debugging
//  std::vector<cv::Point2d> left_debug_points;
//  std::vector<cv::Point2d> right_debug_points;
//  retrieveMatchedPointsAndUpdateMatches(left_points, right_points, &matches, &left_debug_points, &right_debug_points);
//  left_points = left_debug_points;
//  right_points = right_debug_points;
//  timer = ProfilingTimer();
//  auto match_img = ait::Utilities::drawPointMatches(left_input_img.getMat().clone(), left_debug_points, right_input_img.getMat().clone(), right_debug_points);
//  cv::imshow("Keypoint matches", match_img);
//  timer.stopAndPrintTiming("drawing matches");
//  cv::waitKey();

  // Undistort feature points and update keypoints
  timer = ProfilingTimer();
  undistortPoints(left_points, right_points);
  timer.stopAndPrintTiming("undistorting keypoints");

  timer = ProfilingTimer();
  // Filter matches based on epipolar constraint
  std::vector<cv::DMatch> best_matches = filterMatchesWithEpipolarConstraint(matches, left_points, right_points, nullptr, verbose);
  std::vector<cv::Point2d> left_best_points;
  std::vector<cv::Point2d> right_best_points;
  retrieveMatchedPointsAndUpdateMatches(left_points, right_points, &best_matches, &left_best_points, &right_best_points);
  timer.stopAndPrintTiming("filtering matches with epipolar constraint");

  if (best_matches.size() == 0)
  {
    throw Error("All matched keypoints were filtered");
  }

  // For debugging
//  timer = ProfilingTimer();
//  auto best_match_img = ait::Utilities::drawPointMatches(left_input_img.getMat().clone(), left_best_points, right_input_img.getMat().clone(), right_best_points);
//  cv::imshow("Best keypoint matches", best_match_img);
//  timer.stopAndPrintTiming("drawing matches");
//  cv::waitKey();

  timer = ProfilingTimer();
  // Correct matched points based on epipolar constraint
  correctMatches(&left_best_points, &right_best_points);
  timer.stopAndPrintTiming("correcting matches with epipolar constraint");

  // For debugging
//  std::vector<double> epipolar_constraints = matcher.computeEpipolarConstraints(left_best_points, right_best_points);

  // For debugging
//  timer = ProfilingTimer();
  auto left_img_with_keypoints = ait::Utilities::drawPoints(left_input_img.getMat().clone(), left_best_points);
  auto right_img_with_keypoints = ait::Utilities::drawPoints(right_input_img.getMat().clone(), right_best_points);
  cv::imshow("Left keypoints", left_img_with_keypoints);
  cv::imshow("Right keypoints", right_img_with_keypoints);
//  timer.stopAndPrintTiming("drawing keypoints");

  // For debugging
//  timer = ProfilingTimer();
  auto match_img = ait::Utilities::drawPointMatches(left_input_img.getMat().clone(), left_best_points, right_input_img.getMat().clone(), right_best_points);
  cv::imshow("Keypoint matches", match_img);
//  timer.stopAndPrintTiming("drawing matches");

  timer = ProfilingTimer();
  std::vector<cv::Point3d> points_3d = triangulatePoints(left_best_points, right_best_points);
  timer.stopAndPrintTiming("triangulating points");
  timer = ProfilingTimer();
  if (left_image_points != nullptr)
  {
    (*left_image_points) = std::move(left_best_points);
  }
  if (right_image_points != nullptr)
  {
    (*right_image_points) = std::move(right_best_points);
  }
  timer.stopAndPrintTiming("moving image points");

  return points_3d;
}

template <typename T>
SparseMatchResult SparseStereoMatcher<T>::matchFull(const cv::InputArray left_input_img, cv::InputArray right_input_img, bool verbose)
{
  cv::Mat left_img = Utilities::convertToGrayscale(left_input_img);
  cv::Mat right_img = Utilities::convertToGrayscale(right_input_img);
  CV_Assert(left_img.channels() == 1);
  CV_Assert(right_img.channels() == 1);

  // Compute feature points and match them
  std::vector<cv::KeyPoint> left_keypoints;
  std::vector<cv::KeyPoint> right_keypoints;
  cv::Mat left_descriptors;
  cv::Mat right_descriptors;
  std::vector<cv::Point2d> left_points;
  std::vector<cv::Point2d> right_points;
//  feature_detector_->detectAndComputeFeatures(left_img, &left_keypoints, left_descriptors, &left_points);
//  feature_detector_->detectAndComputeFeatures(right_img, &right_keypoints, right_descriptors, &right_points);
  feature_detector_->detectAndComputeFeatures(
      left_img, right_img,
      &left_keypoints, &right_keypoints,
      left_descriptors, right_descriptors,
      &left_points, &right_points);
  std::vector<cv::DMatch> matches = matchFeaturesFlannKnn2(left_descriptors, right_descriptors, ratio_test_threshold_, verbose);

  // For debugging
//  auto left_img_with_keypoints = Utilities::drawKeypoints(left_input_img.getMat().clone(), left_keypoints);
//  auto right_img_with_keypoints = Utilities::drawKeypoints(right_input_img.getMat().clone(), right_keypoints);
//  cv::imshow("Left keypoints", left_img_with_keypoints);
//  cv::imshow("Right keypoints", right_img_with_keypoints);
//
//  auto match_img = Utilities::drawFeatureMatches(left_input_img.getMat().clone(), left_keypoints, right_input_img.getMat().clone(), right_keypoints, matches);
//  cv::imshow("Matches", match_img);

  // Undistort feature points and update keypoints
  cv::Mat left_undist_points = undistortPoints(left_points, calib_.left.camera_matrix, calib_.left.dist_coeffs);
  cv::Mat right_undist_points = undistortPoints(right_points, calib_.right.camera_matrix, calib_.right.dist_coeffs);

  std::vector<cv::KeyPoint> left_undist_keypoints(left_keypoints);
  std::vector<cv::KeyPoint> right_undist_keypoints(right_keypoints);
  for (int i = 0; i < left_undist_keypoints.size(); ++i)
  {
    const cv::Point2d &point = left_undist_points.at<cv::Point2d>(0, i);
    left_undist_keypoints[i].pt = cv::Point2f(point.x, point.y);
  }
  for (int i = 0; i < right_undist_keypoints.size(); ++i)
  {
    const cv::Point2d &point = right_undist_points.at<cv::Point2d>(0, i);
    right_undist_keypoints[i].pt = cv::Point2f(point.x, point.y);
  }

//  // For debugging
//  auto left_img_with_undist_keypoints = Utilities::drawKeypoints(left_input_img.getMat().clone(), left_undist_keypoints);
//  auto right_img_with_undist_keypoints = Utilities::drawKeypoints(right_input_img.getMat().clone(), right_undist_keypoints);
//  cv::imshow("Left undistorted keypoints", left_img_with_undist_keypoints);
//  cv::imshow("Right undistorted keypoints", right_img_with_undist_keypoints);
//
//  auto undist_match_img = Utilities::drawFeatureMatches(left_input_img.getMat().clone(), left_undist_keypoints, right_input_img.getMat().clone(), right_undist_keypoints, matches);
//  cv::imshow("Undistorted matches", undist_match_img);

  // Filter matches based on epipolar constraint
  std::vector<double> epipolar_constraints;
  std::vector<cv::DMatch> best_matches = filterMatchesWithEpipolarConstraint(matches, left_undist_points, right_undist_points, &epipolar_constraints, verbose);
  std::vector<cv::Point2d> left_best_points;
  std::vector<cv::Point2d> right_best_points;
  std::vector<cv::KeyPoint> left_best_keypoints;
  std::vector<cv::KeyPoint> right_best_keypoints;
  retrieveMatchedPointsAndUpdateMatches(left_undist_keypoints, right_undist_keypoints, &best_matches, &left_best_points, &right_best_points, &left_best_keypoints, &right_best_keypoints);

  // For debugging
//  auto best_undist_match_img = Utilities::drawFeatureMatches(left_input_img.getMat().clone(), left_undist_keypoints, right_input_img.getMat().clone(), right_undist_keypoints, best_matches);
//  cv::imshow("Best undistorted matches", best_undist_match_img);

  // Correct matched points based on epipolar constraint
//  correctMatches(&left_best_points, &right_best_points);
  correctMatchesAndUpdateKeypoints(&left_best_points, &right_best_points, &left_best_keypoints, &right_best_keypoints);

  epipolar_constraints = computeEpipolarConstraints(left_best_points, right_best_points);

  // For debugging
//  auto best_match_img = Utilities::drawFeatureMatches(left_input_img.getMat().clone(), left_best_keypoints, right_input_img.getMat().clone(), right_best_keypoints, best_matches);
//  cv::imshow("Best corrected matches", best_match_img);

  // For debugging
//  auto left_img_with_correct_keypoints = Utilities::drawKeypoints(left_input_img.getMat().clone(), left_best_keypoints);
//  auto right_img_with_correct_keypoints = Utilities::drawKeypoints(right_input_img.getMat().clone(), right_best_keypoints);
//  cv::imshow("Left correct keypoints", left_img_with_correct_keypoints);
//  cv::imshow("Right correct keypoints", right_img_with_correct_keypoints);
//
//  cv::waitKey(0);

  std::vector<cv::Point3d> points_3d = triangulatePoints(left_best_points, right_best_points);

  // Copy results to output container
  SparseMatchResult match_result;

  match_result.epipolar_constraints = std::move(epipolar_constraints);

  match_result.points_3d = std::move(points_3d);

  match_result.left_keypoints = std::move(left_best_keypoints);
  match_result.right_keypoints = std::move(right_best_keypoints);

  std::vector<cv::Mat> left_descriptors_vec;
  std::vector<cv::Mat> right_descriptors_vec;
  for (int i = 0; i < best_matches.size(); ++i)
  {
    int left_index = best_matches[i].queryIdx;
    int right_index = best_matches[i].trainIdx;
    left_descriptors_vec.push_back(left_descriptors.row(left_index));
    right_descriptors_vec.push_back(right_descriptors.row(right_index));
  }
  match_result.left_descriptors = std::move(left_descriptors_vec);
  match_result.right_descriptors = std::move(right_descriptors_vec);

  match_result.left_points = std::move(left_best_points);
  match_result.right_points = std::move(right_best_points);

  match_result.matches = std::move(best_matches);

  return match_result;
}

}  // namespace stereo
}  // namespace ait
