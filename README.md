# K-Means Clustering

Language: C++  
Tools: Visual Studio 2019  
External Libraries: lodepng

## Project Description

This is one of the C++ projects I did for the MAT345 Introduction to Data Science course I took during my senior year of college.

I used the K-means algorithm, which groups similar objects together into K clusters, in order to take an image and recolor it using only K colors.

I did this on three different images: An icon size image for quick testing, a 1080P size image with invisible pixels, and another 1080p size image with no invisible pixels.

I also ran the images through the algorithm for each K = [3, 10].

## Result

As the K value increased, the image accuracy also increased. This was expected though, as there are more colors being used to recolor the image.

However, I did not expect the variation in the details between adjacent K values. This can be seen on the Mario images:

When K = 5, only his mustache has shading.  
When K = 6, his boots and side burns have shading but not his mustache  
When K = 7, only his side burns have shading.[^1]

![The original image compared to the recolored images](/OutputImages/Mario.PNG)

![The original image compared to the recolored images](/OutputImages/Toad.PNG)

>*Note: The third image I used, Luigi.png, is not represented here. The resulting images from that image are large and take up multiple pages. You can see those images in the Luigi.pdf file located in the OutputImages directory.*

## Encoding and Decoding Images

To decode the image I used lodepng.h, a simple open source encoding and decoding png library. It decodes the image into an array of bytes, with each image being represent by 4 bytes (RGBA).

I took each pixels raw data and converted it into the pixel data structure so it would be easier to work with.

The structure also contained the previous cluster the pixel was assigned to and the current cluster the pixel is assigned to.

Once I ran the K-means algorithm on the image, I converted the pixels back into raw data form and encoded them into a new image.

```c++
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
```

## K-Means Initialization

Before I can run the main loop of the algorithm, I first have to initialize the means of the K clusters. To do this I found K different pixels that were the farthest away from eachother, and created K clusters from those pixels.

If I didn't do this initialization step, then all pixels would be grouped into a single cluster.

```c++
//Initialize the first mean to a random pixel
  std::random_device device;
  std::mt19937 generator(device());
  std::uniform_int_distribution<> distribution(1, pixels.size());

  means[0] = pixels[distribution(generator)];

//for each other mean, find the pixel that is the farthest from the other means
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
```

## K-Means Loop

The main loop of the K-Means algorithm is simple. For each pixel, calculate the distance from the pixel to each mean of the clusters, and assign the pixel to the cluster that has the shortest distance.

If a pixel cluster assignment changed, then recalculate the means of each cluster and reiterate. Otherwise, the algorithm is finished and all of the pixels in each cluster are closer to eachother than any other pixel.

Finally, I recolor each pixel to the color of the mean of the cluster that pixel is assigned to.

```c++
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
```

## Driver

Since this was for a math course, I shoved all of the driver code into main() for simplicity.

For each K value (3 to 10), decode the image, run K-Means algorithm on image using the current K value, and encode new pixel data into an image.

Finally, clear the pixels vector so its ready for the next iteratation.[^2]

```c++
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
    //set maximum clusters to the current K value
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
```

## Conclusion

In the end, I had a lot of fun with this project. It was a short and simple project but produced some pretty amazing outputs.

Openning up the recolored images was like openning presents on christmas day, and filled me with joy on how closely or differently the recolored images matched the original image.

I enjoyed looking through each image to try and find the differences from eachother and from the original, like a version of the *Where's Waldo?* books.

I learned more about the K-Means algorithm and some good practice with C++.

If I had more time, probably would have optimized it a bit more, then run some 4K images through it with larger K values and see what it produces.

## Footnotes

[^1]: The boots do have extremely faint shading, almost completely invisible. The brown used for his boot shading was probably repurposed to seperate the white of his gloves and bottom of his boot from the tan color of his skin.

[^2]: The pixels vector is the array of the pixels in the image, from top left to bottom right.
