// copyright            : (C) 2018 by Yury
#include <assert.h>
#include <mpi.h>
#include <iostream>
#include <cmath>
#include <cstdlib>
#include <utility>

void simpleOddEvenSort(int arr[], int n) {
    bool isSorted = false;
    while (!isSorted) {
      isSorted = true;
      for (int i = 1; i <= n - 2; i = i + 2) {
        if (arr[i] > arr[i + 1]) {
          std::swap(arr[i], arr[i + 1]);
          isSorted = false;
        }
      }
      for (int i = 0; i <= n - 2; i = i + 2) {
        if (arr[i] > arr[i + 1]) {
          std::swap(arr[i], arr[i + 1]);
          isSorted = false;
        }
      }
    }
}

void mergeAndSplit(int arr1[], int size1, int arr2[], int size2) {
  for (int i = size2 - 1; i >= 0; i--) {
    int j, last = arr1[size1 - 1];
    for (j = size1 - 2; j >= 0 && arr1[j] > arr2[i]; j--)
      arr1[j + 1] = arr1[j];
    if (j != size1 - 2 || last > arr2[i]) {
      arr1[j + 1] = arr2[i];
      arr2[i] = last;
    }
  }
}

void sendAndReceive(int processJuniorId, int * partialArr, int n) {
  // senior by number sends to minor his sorted array
  // and receives his part of merged array
  MPI_Send(partialArr, n, MPI_INT, processJuniorId, 0, MPI_COMM_WORLD);
  MPI_Recv(partialArr, n, MPI_INT, processJuniorId, 0,
    MPI_COMM_WORLD, MPI_STATUSES_IGNORE);
}
void receiveMergeAndSend(
  int processSeniorId,
  int * ownPartialArr,
  int * receivedPartialArr,
  int n
  ) {
  // minor receives and merges
  MPI_Recv(receivedPartialArr, n, MPI_INT, processSeniorId, 0,
    MPI_COMM_WORLD, MPI_STATUSES_IGNORE);
  mergeAndSplit(ownPartialArr, n, receivedPartialArr, n);
  // sends back
  MPI_Send(receivedPartialArr, n, MPI_INT, processSeniorId, 0, MPI_COMM_WORLD);
}
void parallelMerging(
  int procNum,
  int procCount,
  int * ownPartialArr,
  int * bufArr,
  int partialSize
) {
  for (int i = 0; i < procCount; i++) {
    if (i % 2 == 1) {  // odd step
      if (procNum % 2 == 1) {  // odd process
        // exchange with right neighbour
        if (procNum < procCount - 1) {
          receiveMergeAndSend(procNum + 1, ownPartialArr, bufArr, partialSize);
        }
      } else {
        // exchange with left neighbour
        if (procNum > 0) {
          sendAndReceive(procNum - 1, ownPartialArr, partialSize);
        }
      }
    } else {  // even step
      if (procNum % 2 == 0) {  // even process
        // exchange with right neighbour
        if (procNum < procCount - 1) {
          receiveMergeAndSend(procNum + 1, ownPartialArr, bufArr, partialSize);
        }
      } else {
        // exchange with left neighbour
        sendAndReceive(procNum - 1, ownPartialArr, partialSize);
      }
    }
  }
}

int main(int argc, char * argv[]) {
  int status = 0, rank = 0, size = 0;
  int x = atoi(argv[1]);
  double starttime2, endtime2, starttime1, endtime1;

  status = MPI_Init(& argc, & argv);
  // assert(status == MPI_SUCCESS);
  if (status != MPI_SUCCESS) {
    std::cout << "ERROR\n";
    return 1;
  }

  status = MPI_Comm_rank(MPI_COMM_WORLD, & rank);
  // assert(status == MPI_SUCCESS);
  if (status != MPI_SUCCESS) {
    std::cout << "ERROR\n";
    return 1;
  }

  status = MPI_Comm_size(MPI_COMM_WORLD, & size);
  // assert(status == MPI_SUCCESS);
  if (status != MPI_SUCCESS) {
    std::cout << "ERROR\n";
    return 1;
  }
  // buf array for receiving data
  // from major process
  int * bufArr = new int[x];

  if (rank == 0) {
    // Sequence version
    // create
    int * arrS = new int[x * size];
    int seed = (int)(MPI_Wtime());
    std::srand(seed);
    for (int i = 0; i < x * size; i++) {
      arrS[i] = std::rand();
    }
    ////////////////////////////////////
    starttime1 = MPI_Wtime();
    simpleOddEvenSort(arrS, x * size);
    endtime1 = MPI_Wtime();
    ////////////////////////////////////
    // Parallel version
    // create
    int * arr1 = new int[x * size];
    seed = (int)(MPI_Wtime());
    std::srand(seed);
    for (int i = 0; i < x * size; i++) {
      arr1[i] = std::rand();
    }
    // print
    if (size * x < 100) {
      for (int i = 0; i < x * size; i++) {
      std::cout << "arr[" << i << "] = \t" << arr1[i] << "\n";
      }
    }

    /////////////////////////////////////// count1
    starttime2 = MPI_Wtime();
    // send data
    for (int i = 1; i <= size - 1; i++) {
      MPI_Send(arr1 + x * i, x, MPI_INT, i, 0, MPI_COMM_WORLD);
    }
    // local sort
    simpleOddEvenSort(arr1, x);

    // merge
    parallelMerging(rank, size, arr1, bufArr, x);

    // receiving
    for (int i = 1; i <= size - 1; i++) {
      MPI_Recv(arr1 + x * i, x, MPI_INT, i, 0,
        MPI_COMM_WORLD, MPI_STATUSES_IGNORE);
    }
    endtime2 = MPI_Wtime();
    ///////////////////////////////////////
    // checking
    bool sorted = true;
    for (int i = 0; i < x * size - 1; i++) {
      if (arr1[i] > arr1[i + 1]) {
        sorted = false;
        break;
      }
    }
    // print
    if (size * x < 100) {
      for (int i = 0; i < x * size; i++) {
      std::cout << "Sorted arr[" << i << "] = \t" << arr1[i] << "\n";
      }
    }
    std::cout << "Array sorted" <<
      (sorted ? " rightly" : " unrightly") << "\n";
    std::cout << "Parallel time = " <<
      endtime2 - starttime2 << "\n";
    std::cout << "Sequence time = " <<
      endtime1 - starttime1 << "\n";
      delete[] arr1;
      delete[] arrS;
  }
  if (rank != 0) {
    // receiving data
    int * partialArr1 = new int[x];
    MPI_Recv(partialArr1, x, MPI_INT, 0, 0,
      MPI_COMM_WORLD, MPI_STATUSES_IGNORE);
    // local sort
    simpleOddEvenSort(partialArr1, x);
    // merge
    parallelMerging(rank, size, partialArr1, bufArr, x);

    // send back
    MPI_Send(partialArr1, x, MPI_INT, 0, 0, MPI_COMM_WORLD);
    delete[] partialArr1;
  }

  status = MPI_Finalize();
  assert(status == MPI_SUCCESS);
  delete[] bufArr;
  return 0;
}