#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <map>
#include <random>
#include "lodepng.h" //3rd party library for decoding and encoding png files

//maximum number of clusters to group pixels into
unsigned int maxClusters;

//pixel data structure used to convert raw data into usable data
struct Pixel
{
  unsigned int red;
  unsigned int green;
  unsigned int blue;
  unsigned int alpha;
  unsigned int previousCluster;
  unsigned int currentCluster;

  Pixel() : red(0), green(0), blue(0), alpha(0), previousCluster(0), currentCluster(0) {}
  Pixel(unsigned char newRed, unsigned char newGreen, unsigned char newBlue, unsigned char newAlpha) : red(newRed), green(newGreen), blue(newBlue), alpha(newAlpha), previousCluster(0), currentCluster(0) {}
};

//vector of all pixels, ordered from top left to bottom right, columns then rows
std::vector<Pixel> pixels;

//width of the image
unsigned imageWidth;

//height of the image
unsigned imageHeight;

//decode the image and store pixel data into pixels vector. returns true for success and false for failure
bool DecodeImage(std::string filename)
{
  //pixels from the input image
  std::vector<unsigned char> inputImage;

  //decode input image and store each value into inputImage vector, order RGBARGBARGBA...
  std::cout << "decoding image..." << std::endl;
  unsigned error = lodepng::decode(inputImage, imageWidth, imageHeight, filename);

  //if there was an error then print the error and return failure
  if (error)
  {
    std::cout << "decoder error " << error << ": " << lodepng_error_text(error) << std::endl;
    return false;
  }

  //convert raw data into pixel structure and add it the pixel vector
  for (int i = 0; i < imageWidth * imageHeight * 4; i += 4)
  {
    pixels.emplace_back(inputImage[i], inputImage[i + 1], inputImage[i + 2], inputImage[i+3]);
  }

  //return success
  return true;
}

//encodes the pixel data vector into a png. retunrs true for success and false for failure
bool EncodeImage()
{
  //pixels to output
  std::vector<unsigned char> outputImage;

  //convert pixel structure into raw data
  for (const Pixel& pixel : pixels)
  {
    outputImage.push_back(pixel.red);
    outputImage.push_back(pixel.green);
    outputImage.push_back(pixel.blue);
    outputImage.push_back(pixel.alpha);
  }

  //encode the raw data into a png
  std::cout << "encoding image..." << std::endl;
  unsigned error = lodepng::encode("output" + std::to_string(maxClusters) + ".png", outputImage, imageWidth, imageHeight);

  //if there was an error then print the error and return failure
  if (error)
  {
    std::cout << "encoder error " << error << ": " << lodepng_error_text(error) << std::endl;
    return false;
  }

  //return success
  return true;
}

//calculate the squared distance between two pixels
unsigned int DistanceSquared(const Pixel& pixel1, const Pixel& pixel2)
{
  return (pixel1.red - pixel2.red) * (pixel1.red - pixel2.red) + (pixel1.green - pixel2.green) * (pixel1.green - pixel2.green) + (pixel1.blue - pixel2.blue) * (pixel1.blue - pixel2.blue);
}


//calculates a pixel that is the mean of all of the pixels in the pixel cluster
Pixel CalculateMean(const std::vector<Pixel>& pixelCluster)
{
  //if the pixel cluster is emtpy, return a black pixel
  if (pixelCluster.size() == 0)
  {
    return Pixel();
  }

  Pixel pixelMean;

  //add each pixel RGD value to the mean of the cluster
  for (const Pixel& pixel : pixelCluster)
  {
    pixelMean.red += pixel.red;
    pixelMean.green += pixel.green;
    pixelMean.blue += pixel.blue;
  }

  //divide RGB values by the number of pixels in the cluster
  pixelMean.red /= pixelCluster.size();
  pixelMean.green /= pixelCluster.size();
  pixelMean.blue /= pixelCluster.size();

  //return the mean of the cluster
  return pixelMean;
}

//k-means algorithm
void KMeans()
{
  //is true iff a pixel was assigned to a different cluster
  bool clusterAssignmentChanged = true;
  std::vector<Pixel> means(maxClusters);
  std::vector<std::vector<Pixel>> clusters(maxClusters);

  std::cout << "Running k-means algorithm..." << std::endl;

  //Initialize the first mean to a random pixel
  std::random_device device;
  std::mt19937 generator(device());
  std::uniform_int_distribution<> distribution(1, pixels.size());

  means[0] = pixels[distribution(generator)];

  //for eachother mean, find the pixel that is the farthest from the other means
  for (int i = 1; i < maxClusters; i++)
  {
    //farthest average distance and pixel
    unsigned int largestAverageDistance = 0;
    Pixel farthestPixel;

    //for each pixel, calculate average distance
    for (Pixel pixel : pixels)
    {
      //don't calculate average distance for invisible pixels
      if (pixel.alpha == 0)
      {
        continue;
      }

      unsigned int averageDistance = 0;

      for (int j = 0; j < i; j++)
      {
        averageDistance += DistanceSquared(pixel, means[j]);
      }

      averageDistance /= i;

      //if average distance is larger than the largest average distance, then swap farthest distance and pixel
      if (averageDistance > largestAverageDistance)
      {
        largestAverageDistance = averageDistance;
        farthestPixel = pixel;
      }
    }

    //set mean to farthest pixel
    means[i] = farthestPixel;
  }

  //while cluster assignements have changed, compute kmeans algorithm
  while (clusterAssignmentChanged == true)
  {
    //reset cluster assignment
    clusterAssignmentChanged = false;

    //clear last iteration clusters
    for (std::vector<Pixel>& cluster : clusters)
    {
      cluster.clear();
    }

    //for each pixel, assign the cluster the pixel is closest to
    for (Pixel& pixel : pixels)
    {
      //skip pixels that are invisible
      if (pixel.alpha == 0)
      {
        continue;
      }

      //initialize smallest distance and cluster assignment
      unsigned int smallestDistance = std::numeric_limits<unsigned int>().max();
      unsigned int clusterAssignment = std::numeric_limits<unsigned int>().max();

      //for each cluster, calculate shortest distance and cluster assignment
      for (unsigned int i = 0; i < maxClusters; i++)
      {
        unsigned int distance = DistanceSquared(pixel, means[i]);
        if (distance < smallestDistance)
        {
          smallestDistance = distance;
          clusterAssignment = i;
        }
      }

      //add pixel to cluster
      clusters[clusterAssignment].push_back(pixel);

      //update cluster assignment
      pixel.previousCluster = pixel.currentCluster;
      pixel.currentCluster = clusterAssignment;

      if (pixel.previousCluster != pixel.currentCluster)
      {
        clusterAssignmentChanged = true;
      }
    }

    //if cluster assignment has changed, recompute means
    if (clusterAssignmentChanged == true)
    {
      for (int i = 0; i < maxClusters; i++)
      {
        means[i] = CalculateMean(clusters[i]);
      }
    }
  }

  //recolor pixels based on cluster
  for (Pixel& pixel : pixels)
  {
    //dont recolor pixels that are invisible
    if (pixel.alpha == 0)
    {
      continue;
    }

    pixel = means[pixel.currentCluster];
    pixel.alpha = 255;
  }
}

//driver
int main()
{
  //get the filename from user
  std::string filename;
  std::cout << "input filename" << std::endl;
  std::cin >> filename;

  //for each k value, run k means algorithm on image using k value
  for (int i = 3; i <= 10; i++)
  {
    maxClusters = i;

    //if decoding failed, then return failure
    if (DecodeImage(filename) == false)
    {
      return 1;
    }

    //run K-means algorithm
    KMeans();

    //if encoding failed, then return failure
    if (EncodeImage() == false)
    {
      return 1;
    }

    //reset pixels vector for next image
    pixels.clear();
  }
}