#include "hungarianAlg.h"
#include <limits>
#include <iostream>
#include <limits>
#include <cassert>

track_t AssignmentProblemSolver::Solve(
  const distMatrix_t& distMatrixIn,
  size_t nOfRows,
  size_t nOfColumns,
  std::vector<int>& assignment,
  TMethod Method
  )
{
  assignment.resize(nOfRows, -1);

  track_t cost = 0;

  switch (Method)
  {
    case optimal:
      assignmentoptimal(assignment, cost, distMatrixIn, nOfRows, nOfColumns);
      break;

    case many_forbidden_assignments:
      assignmentsuboptimal1(assignment, cost, distMatrixIn, nOfRows, nOfColumns);
      break;

    case without_forbidden_assignments:
      assignmentsuboptimal2(assignment, cost, distMatrixIn, nOfRows, nOfColumns);
      break;
  }

  return cost;
}

// --------------------------------------------------------------------------
// Computes the optimal assignment (minimum overall costs) using Munkres algorithm.
// --------------------------------------------------------------------------
void AssignmentProblemSolver::assignmentoptimal(assignments_t& assignment, track_t& cost, const distMatrix_t& distMatrixIn, size_t nOfRows, size_t nOfColumns)
{
  // Generate distance cv::Matrix 
  // and check cv::Matrix elements positiveness :)

  // Total elements number
  const auto nOfElements = nOfRows * nOfColumns;
  // Memory allocation
  const auto distMatrix = new track_t[nOfElements]();
  // Pointer to last element
  const auto distMatrixEnd = distMatrix + nOfElements;

  for (size_t row = 0; row < nOfElements; row++)
  {
    const auto value = distMatrixIn[row];
    assert(value >= 0);
    distMatrix[row] = value;
  }

  // Memory allocation
  const auto coveredColumns = new bool[nOfColumns]();
  const auto coveredRows = new bool[nOfRows]();
  const auto starMatrix = new bool[nOfElements]();
  const auto primeMatrix = new bool[nOfElements]();
  const auto newStarMatrix = new bool[nOfElements](); /* used in step4 */

  /* preliminary steps */
  if (nOfRows <= nOfColumns)
  {
    for (size_t row = 0; row < nOfRows; row++)
    {
      /* find the smallest element in the row */
      auto distMatrixTemp = distMatrix + row;
      auto minValue = *distMatrixTemp;
      distMatrixTemp += nOfRows;
      while (distMatrixTemp < distMatrixEnd)
      {
        const auto value = *distMatrixTemp;
        if (value < minValue)
        {
          minValue = value;
        }
        distMatrixTemp += nOfRows;
      }
      /* subtract the smallest element from each element of the row */
      distMatrixTemp = distMatrix + row;
      while (distMatrixTemp < distMatrixEnd)
      {
        *distMatrixTemp -= minValue;
        distMatrixTemp += nOfRows;
      }
    }
    /* Steps 1 and 2a */
    for (size_t row = 0; row < nOfRows; row++)
    {
      for (size_t col = 0; col < nOfColumns; col++)
      {
        if (distMatrix[row + nOfRows * col] == 0)
        {
          if (!coveredColumns[col])
          {
            starMatrix[row + nOfRows * col] = true;
            coveredColumns[col] = true;
            break;
          }
        }
      }
    }
  }
  else /* if(nOfRows > nOfColumns) */
  {
    for (size_t col = 0; col < nOfColumns; col++)
    {
      /* find the smallest element in the column */
      auto distMatrixTemp = distMatrix + nOfRows * col;
      const auto columnEnd = distMatrixTemp + nOfRows;
      auto minValue = *distMatrixTemp++;
      while (distMatrixTemp < columnEnd)
      {
        const auto value = *distMatrixTemp++;
        if (value < minValue)
        {
          minValue = value;
        }
      }
      /* subtract the smallest element from each element of the column */
      distMatrixTemp = distMatrix + nOfRows * col;
      while (distMatrixTemp < columnEnd)
      {
        *distMatrixTemp++ -= minValue;
      }
    }
    /* Steps 1 and 2a */
    for (size_t col = 0; col < nOfColumns; col++)
    {
      for (size_t row = 0; row < nOfRows; row++)
      {
        if (distMatrix[row + nOfRows * col] == 0)
        {
          if (!coveredRows[row])
          {
            starMatrix[row + nOfRows * col] = true;
            coveredColumns[col] = true;
            coveredRows[row] = true;
            break;
          }
        }
      }
    }

    for (size_t row = 0; row < nOfRows; row++)
    {
      coveredRows[row] = false;
    }
  }
  /* move to step 2b */
  step2b(assignment, distMatrix, starMatrix, newStarMatrix, primeMatrix, coveredColumns, coveredRows, nOfRows, nOfColumns, (nOfRows <= nOfColumns) ? nOfRows : nOfColumns);
  /* compute cost and remove invalid assignments */
  computeassignmentcost(assignment, cost, distMatrixIn, nOfRows);
  /* free allocated memory */
  delete[] distMatrix;
  delete[] coveredColumns;
  delete[] coveredRows;
  delete[] starMatrix;
  delete[] primeMatrix;
  delete[] newStarMatrix;
  return;
}

// --------------------------------------------------------------------------
//
// --------------------------------------------------------------------------
void AssignmentProblemSolver::buildassignmentvector(assignments_t& assignment, bool* starMatrix, size_t nOfRows, size_t nOfColumns) const
{
  for (size_t row = 0; row < nOfRows; row++)
  {
    for (size_t col = 0; col < nOfColumns; col++)
    {
      if (starMatrix[row + nOfRows * col])
      {
        assignment[row] = static_cast<int>(col);
        break;
      }
    }
  }
}

// --------------------------------------------------------------------------
//
// --------------------------------------------------------------------------
void AssignmentProblemSolver::computeassignmentcost(const assignments_t& assignment, track_t& cost, const distMatrix_t& distMatrixIn, size_t nOfRows) const
{
  for (size_t row = 0; row < nOfRows; row++)
  {
    const auto col = assignment[row];
    if (col >= 0)
    {
      cost += distMatrixIn[row + nOfRows * col];
    }
  }
}

// --------------------------------------------------------------------------
//
// --------------------------------------------------------------------------
void AssignmentProblemSolver::step2a(assignments_t& assignment, track_t* distMatrix, bool* starMatrix, bool* newStarMatrix, bool* primeMatrix, bool* coveredColumns, bool* coveredRows, size_t nOfRows, size_t nOfColumns, size_t minDim)
{
  /* cover every column containing a starred zero */
  for (size_t col = 0; col < nOfColumns; col++)
  {
    auto starMatrixTemp = starMatrix + nOfRows * col;
    const auto columnEnd = starMatrixTemp + nOfRows;
    while (starMatrixTemp < columnEnd)
    {
      if (*starMatrixTemp++)
      {
        coveredColumns[col] = true;
        break;
      }
    }
  }
  /* move to step 3 */
  step2b(assignment, distMatrix, starMatrix, newStarMatrix, primeMatrix, coveredColumns, coveredRows, nOfRows, nOfColumns, minDim);
}

// --------------------------------------------------------------------------
//
// --------------------------------------------------------------------------
void AssignmentProblemSolver::step2b(assignments_t& assignment, track_t* distMatrix, bool* starMatrix, bool* newStarMatrix, bool* primeMatrix, bool* coveredColumns, bool* coveredRows, size_t nOfRows, size_t nOfColumns, size_t minDim)
{
  /* count covered columns */
  size_t nOfCoveredColumns = 0;
  for (size_t col = 0; col < nOfColumns; col++)
  {
    if (coveredColumns[col])
    {
      nOfCoveredColumns++;
    }
  }
  if (nOfCoveredColumns == minDim)
  {
    /* algorithm finished */
    buildassignmentvector(assignment, starMatrix, nOfRows, nOfColumns);
  }
  else
  {
    /* move to step 3 */
    step3(assignment, distMatrix, starMatrix, newStarMatrix, primeMatrix, coveredColumns, coveredRows, nOfRows, nOfColumns, minDim);
  }
}

// --------------------------------------------------------------------------
//
// --------------------------------------------------------------------------
void AssignmentProblemSolver::step3(assignments_t& assignment, track_t* distMatrix, bool* starMatrix, bool* newStarMatrix, bool* primeMatrix, bool* coveredColumns, bool* coveredRows, size_t nOfRows, size_t nOfColumns, size_t minDim)
{
  auto zerosFound = true;
  while (zerosFound)
  {
    zerosFound = false;
    for (size_t col = 0; col < nOfColumns; col++)
    {
      if (!coveredColumns[col])
      {
        for (size_t row = 0; row < nOfRows; row++)
        {
          if ((!coveredRows[row]) && (distMatrix[row + nOfRows * col] == 0))
          {
            /* prime zero */
            primeMatrix[row + nOfRows * col] = true;
            /* find starred zero in current row */
            size_t starCol = 0;
            for (; starCol < nOfColumns; starCol++)
            {
              if (starMatrix[row + nOfRows * starCol])
              {
                break;
              }
            }
            if (starCol == nOfColumns) /* no starred zero found */
            {
              /* move to step 4 */
              step4(assignment, distMatrix, starMatrix, newStarMatrix, primeMatrix, coveredColumns, coveredRows, nOfRows, nOfColumns, minDim, row, col);
              return;
            }
            else
            {
              coveredRows[row] = true;
              coveredColumns[starCol] = false;
              zerosFound = true;
              break;
            }
          }
        }
      }
    }
  }
  /* move to step 5 */
  step5(assignment, distMatrix, starMatrix, newStarMatrix, primeMatrix, coveredColumns, coveredRows, nOfRows, nOfColumns, minDim);
}

// --------------------------------------------------------------------------
//
// --------------------------------------------------------------------------
void AssignmentProblemSolver::step4(assignments_t& assignment, track_t* distMatrix, bool* starMatrix, bool* newStarMatrix, bool* primeMatrix, bool* coveredColumns, bool* coveredRows, size_t nOfRows, size_t nOfColumns, size_t minDim, size_t row, size_t col)
{
  const auto nOfElements = nOfRows * nOfColumns;
  /* generate temporary copy of starMatrix */
  for (size_t n = 0; n < nOfElements; n++)
  {
    newStarMatrix[n] = starMatrix[n];
  }
  /* star current zero */
  newStarMatrix[row + nOfRows * col] = true;
  /* find starred zero in current column */
  auto starCol = col;
  size_t starRow = 0;
  for (; starRow < nOfRows; starRow++)
  {
    if (starMatrix[starRow + nOfRows * starCol])
    {
      break;
    }
  }
  while (starRow < nOfRows)
  {
    /* unstar the starred zero */
    newStarMatrix[starRow + nOfRows * starCol] = false;
    /* find primed zero in current row */
    const auto primeRow = starRow;
    size_t primeCol = 0;
    for (; primeCol < nOfColumns; primeCol++)
    {
      if (primeMatrix[primeRow + nOfRows * primeCol])
      {
        break;
      }
    }
    /* star the primed zero */
    newStarMatrix[primeRow + nOfRows * primeCol] = true;
    /* find starred zero in current column */
    starCol = primeCol;
    for (starRow = 0; starRow < nOfRows; starRow++)
    {
      if (starMatrix[starRow + nOfRows * starCol])
      {
        break;
      }
    }
  }
  /* use temporary copy as new starMatrix */
  /* delete all primes, uncover all rows */
  for (size_t n = 0; n < nOfElements; n++)
  {
    primeMatrix[n] = false;
    starMatrix[n] = newStarMatrix[n];
  }
  for (size_t n = 0; n < nOfRows; n++)
  {
    coveredRows[n] = false;
  }
  /* move to step 2a */
  step2a(assignment, distMatrix, starMatrix, newStarMatrix, primeMatrix, coveredColumns, coveredRows, nOfRows, nOfColumns, minDim);
}

// --------------------------------------------------------------------------
//
// --------------------------------------------------------------------------
void AssignmentProblemSolver::step5(assignments_t& assignment, track_t* distMatrix, bool* starMatrix, bool* newStarMatrix, bool* primeMatrix, bool* coveredColumns, bool* coveredRows, size_t nOfRows, size_t nOfColumns, size_t minDim)
{
  /* find smallest uncovered element h */
  auto h = std::numeric_limits<track_t>::max();
  for (size_t row = 0; row < nOfRows; row++)
  {
    if (!coveredRows[row])
    {
      for (size_t col = 0; col < nOfColumns; col++)
      {
        if (!coveredColumns[col])
        {
          const float value = distMatrix[row + nOfRows * col];
          if (value < h)
          {
            h = value;
          }
        }
      }
    }
  }
  /* add h to each covered row */
  for (size_t row = 0; row < nOfRows; row++)
  {
    if (coveredRows[row])
    {
      for (size_t col = 0; col < nOfColumns; col++)
      {
        distMatrix[row + nOfRows * col] += h;
      }
    }
  }
  /* subtract h from each uncovered column */
  for (size_t col = 0; col < nOfColumns; col++)
  {
    if (!coveredColumns[col])
    {
      for (size_t row = 0; row < nOfRows; row++)
      {
        distMatrix[row + nOfRows * col] -= h;
      }
    }
  }
  /* move to step 3 */
  step3(assignment, distMatrix, starMatrix, newStarMatrix, primeMatrix, coveredColumns, coveredRows, nOfRows, nOfColumns, minDim);
}


// --------------------------------------------------------------------------
// Computes a suboptimal solution. Good for cases without forbidden assignments.
// --------------------------------------------------------------------------
void AssignmentProblemSolver::assignmentsuboptimal2(assignments_t& assignment, track_t& cost, const distMatrix_t& distMatrixIn, size_t nOfRows, size_t nOfColumns)
{
  /* make working copy of distance Matrix */
  const auto nOfElements = nOfRows * nOfColumns;
  const auto distMatrix = new float[nOfElements]();
  for (size_t n = 0; n < nOfElements; n++)
  {
    distMatrix[n] = distMatrixIn[n];
  }

  /* recursively search for the minimum element and do the assignment */
  for (;;)
  {
    /* find minimum distance observation-to-track pair */
    float minValue = std::numeric_limits<track_t>::max();
    size_t tmpRow = 0;
    size_t tmpCol = 0;
    for (size_t row = 0; row < nOfRows; row++)
    {
      for (size_t col = 0; col < nOfColumns; col++)
      {
        const float value = distMatrix[row + nOfRows * col];
        if (value != std::numeric_limits<track_t>::max() && (value < minValue))
        {
          minValue = value;
          tmpRow = row;
          tmpCol = col;
        }
      }
    }

    if (minValue != std::numeric_limits<track_t>::max())
    {
      assignment[tmpRow] = static_cast<int>(tmpCol);
      cost += minValue;
      for (size_t n = 0; n < nOfRows; n++)
      {
        distMatrix[n + nOfRows * tmpCol] = std::numeric_limits<track_t>::max();
      }
      for (size_t n = 0; n < nOfColumns; n++)
      {
        distMatrix[tmpRow + nOfRows * n] = std::numeric_limits<track_t>::max();
      }
    }
    else
    {
      break;
    }
  }

  delete[] distMatrix;
}

// --------------------------------------------------------------------------
// Computes a suboptimal solution. Good for cases with many forbidden assignments.
// --------------------------------------------------------------------------
void AssignmentProblemSolver::assignmentsuboptimal1(assignments_t& assignment, track_t& cost, const distMatrix_t& distMatrixIn, size_t nOfRows, size_t nOfColumns)
{
  /* make working copy of distance Matrix */
  const auto nOfElements = nOfRows * nOfColumns;
  const auto distMatrix = new float[nOfElements]();
  for (size_t n = 0; n < nOfElements; n++)
  {
    distMatrix[n] = distMatrixIn[n];
  }

  /* allocate memory */
  const auto nOfValidObservations = new int[nOfRows]();
  const auto nOfValidTracks = new int[nOfColumns]();

  /* compute number of validations */
  auto infiniteValueFound = false;
  auto finiteValueFound = false;
  for (size_t row = 0; row < nOfRows; row++)
  {
    for (size_t col = 0; col < nOfColumns; col++)
    {
      if (distMatrix[row + nOfRows * col] != std::numeric_limits<track_t>::max())
      {
        nOfValidTracks[col] += 1;
        nOfValidObservations[row] += 1;
        finiteValueFound = true;
      }
      else
      {
        infiniteValueFound = true;
      }
    }
  }

  if (infiniteValueFound)
  {
    if (!finiteValueFound)
    {
      return;
    }
    bool repeatSteps = true;

    while (repeatSteps)
    {
      repeatSteps = false;

      /* step 1: reject assignments of multiply validated tracks to singly validated observations		 */
      for (size_t col = 0; col < nOfColumns; col++)
      {
        for (size_t row = 0; row < nOfRows; row++)
        {
          if (distMatrix[row + nOfRows * col] != std::numeric_limits<track_t>::max() && (nOfValidObservations[row] == 1))
          {
            break;
          }
        }
      }

      /* step 2: reject assignments of multiply validated observations to singly validated tracks */
      if (nOfColumns > 1)
      {
        for (size_t row = 0; row < nOfRows; row++)
        {
          auto singleValidationFound = false;
          for (size_t col = 0; col < nOfColumns; col++)
          {
            if (distMatrix[row + nOfRows * col] != std::numeric_limits<track_t>::max() && (nOfValidTracks[col] == 1))
            {
              singleValidationFound = true;
              break;
            }
          }

          if (singleValidationFound)
          {
            for (size_t col = 0; col < nOfColumns; col++)
            {
              if ((nOfValidTracks[col] > 1) && distMatrix[row + nOfRows * col] != std::numeric_limits<track_t>::max())
              {
                distMatrix[row + nOfRows * col] = std::numeric_limits<track_t>::max();
                nOfValidObservations[row] -= 1;
                nOfValidTracks[col] -= 1;
                repeatSteps = true;
              }
            }
          }
        }
      }
    } /* while(repeatSteps) */

    /* for each multiply validated track that validates only with singly validated  */
    /* observations, choose the observation with minimum distance */
    for (size_t row = 0; row < nOfRows; row++)
    {
      if (nOfValidObservations[row] > 1)
      {
        auto allSinglyValidated = true;
        auto minValue = std::numeric_limits<track_t>::max();
        size_t tmpCol = 0;
        for (size_t col = 0; col < nOfColumns; col++)
        {
          const auto value = distMatrix[row + nOfRows * col];
          if (value != std::numeric_limits<track_t>::max())
          {
            if (nOfValidTracks[col] > 1)
            {
              allSinglyValidated = false;
              break;
            }
            else if ((nOfValidTracks[col] == 1) && (value < minValue))
            {
              tmpCol = col;
              minValue = value;
            }
          }
        }

        if (allSinglyValidated)
        {
          assignment[row] = static_cast<int>(tmpCol);
          cost += minValue;
          for (size_t n = 0; n < nOfRows; n++)
          {
            distMatrix[n + nOfRows * tmpCol] = std::numeric_limits<track_t>::max();
          }
          for (size_t n = 0; n < nOfColumns; n++)
          {
            distMatrix[row + nOfRows * n] = std::numeric_limits<track_t>::max();
          }
        }
      }
    }

    // for each multiply validated observation that validates only with singly validated  track, choose the track with minimum distance
    for (size_t col = 0; col < nOfColumns; col++)
    {
      if (nOfValidTracks[col] > 1)
      {
        bool allSinglyValidated = true;
        float minValue = std::numeric_limits<track_t>::max();
        size_t tmpRow = 0;
        for (size_t row = 0; row < nOfRows; row++)
        {
          const float value = distMatrix[row + nOfRows * col];
          if (value != std::numeric_limits<track_t>::max())
          {
            if (nOfValidObservations[row] > 1)
            {
              allSinglyValidated = false;
              break;
            }
            else if ((nOfValidObservations[row] == 1) && (value < minValue))
            {
              tmpRow = row;
              minValue = value;
            }
          }
        }

        if (allSinglyValidated)
        {
          assignment[tmpRow] = static_cast<int>(col);
          cost += minValue;
          for (size_t n = 0; n < nOfRows; n++)
          {
            distMatrix[n + nOfRows * col] = std::numeric_limits<track_t>::max();
          }
          for (size_t n = 0; n < nOfColumns; n++)
          {
            distMatrix[tmpRow + nOfRows * n] = std::numeric_limits<track_t>::max();
          }
        }
      }
    }
  } /* if(infiniteValueFound) */


  /* now, recursively search for the minimum element and do the assignment */
  for (;;)
  {
    /* find minimum distance observation-to-track pair */
    auto minValue = std::numeric_limits<track_t>::max();
    size_t tmpRow = 0;
    size_t tmpCol = 0;
    for (size_t row = 0; row < nOfRows; row++)
    {
      for (size_t col = 0; col < nOfColumns; col++)
      {
        const auto value = distMatrix[row + nOfRows * col];
        if (value != std::numeric_limits<track_t>::max() && (value < minValue))
        {
          minValue = value;
          tmpRow = row;
          tmpCol = col;
        }
      }
    }

    if (minValue != std::numeric_limits<track_t>::max())
    {
      assignment[tmpRow] = static_cast<int>(tmpCol);
      cost += minValue;
      for (size_t n = 0; n < nOfRows; n++)
      {
        distMatrix[n + nOfRows * tmpCol] = std::numeric_limits<track_t>::max();
      }
      for (size_t n = 0; n < nOfColumns; n++)
      {
        distMatrix[tmpRow + nOfRows * n] = std::numeric_limits<track_t>::max();
      }
    }
    else
    {
      break;
    }
  }

  /* free allocated memory */
  delete[] nOfValidObservations;
  delete[] nOfValidTracks;
  delete[] distMatrix;
}
