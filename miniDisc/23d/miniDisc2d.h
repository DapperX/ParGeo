// Copyright (c) 2020 Yiqiu Wang and the Pargeo Team
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the
// "Software"), to deal in the Software without restriction, including
// without limitation the rights (to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to
// the following conditions:
//
// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
// LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
// OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
// WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

#ifndef MINI_DISC_2D_H
#define MINI_DISC_2D_H

#include "pbbs/utils.h"
#include "pbbs/sequence.h"
#include "geometry.h"
#include "check.h"
#include "prefix.h"

circle miniDisc2DSerial(point<2>* P, intT n, point<2> pi, point<2> pj) {
  typedef circle circleT;
  auto circle = circleT(pi, pj);

  for (intT i=0; i<n; ++i) {
    if (!circle.contain(P[i])) {
      circle = circleT(pi, pj, P[i]);
      swap(P[0], P[i]);
      //swap(P[100 + rand() % 100], P[i]);
    }}
  return circle;
}

circle miniDisc2DSerial(point<2>* P, intT n, point<2> pi) {
  typedef circle circleT;

  auto circle = circleT(P[0], pi);
  for (intT j=1; j<n; ++j) {
    if (!circle.contain(P[j])) {
      circle = miniDisc2DSerial(P, j, pi, P[j]);
      swap(P[1], P[j]);
      //swap(P[20 + rand() % 80], P[j]);
    }
  }
  return circle;
}

circle miniDisc2DSerial(point<2>* P, intT n) {
  typedef circle circleT;

  auto circle = circleT(P[0], P[1]);
  for (intT i=2; i<n; ++i) {
    if (!circle.contain(P[i])) {
      cout << "ci = " << i << endl;
      circle = miniDisc2DSerial(P, i, P[i]);
      swap(P[2], P[i]);
      //swap(P[rand() % 20], P[i]);
    }
  }
  return circle;
}

//take ``vertical'' line (i,j) and get ``left/right'' most circle center
circle miniDisc2DParallel2(point<2>* P, intT n, point<2> pi, point<2> pj, intT* flag=NULL) {
  typedef point<2> pointT;
  typedef circle circleT;

  auto circle = circleT(pi, pj);

  if (pi[1] == pj[1]) {
    abort();
  } else {
    bool freeFlag = false;
    if(!flag) {
      flag = newA(intT, n+1);//marks left/right to (pi,pj)
      freeFlag = true;}

    bool inputOk = true;
    par_for (intT jj=0; jj<n; ++jj) {
      if (!circle.contain(P[jj])) inputOk = false;
    }
    if (inputOk) return circle;

    pointT avg = pi.average(pj);
    floatT slope0 = (pi[1]-pj[1]) / (pi[0]-pj[0]);
    floatT offset0 = avg[1] - slope0*avg[0];

    auto centers = newA(pointT, n);
    par_for (intT jj=0; jj<n; ++jj) {
      auto c = circleT(pi, pj, P[jj]);

      //left/right
      floatT y = slope0*P[jj][0] + offset0;//y intersept with (pi,pj)
      if ((y < P[jj][1] && slope0 >= 0) || (y >= P[jj][1] && slope0 < 0)) {
        flag[jj] = 0;
      } else flag[jj] = 1;

      centers[jj] = c.center();
    }

    //left/right center extrema from left/right points
    struct getL {
      pointT* in; intT* flag;
      getL(pointT* inn, intT* flagg): in(inn), flag(flagg) {};
      floatT operator() (intT idx) {
        if (flag[idx] == 0) return in[idx][0];
        else return floatMax();
      }};
    struct getR {
      pointT* in; intT* flag;
      getR(pointT* inn, intT* flagg): in(inn), flag(flagg) {};
      floatT operator() (intT idx) {
        if (flag[idx] == 1) return in[idx][0];
        else return floatMin();
      }};

    intT iMin = sequence::minIndex<floatT, intT, getL>(0, n, getL(centers, flag));
    intT iMax = sequence::maxIndex<floatT, intT, getR>(0, n, getR(centers, flag));
    auto circle1 = circleT(pi, pj, P[iMin]);
    auto circle2 = circleT(pi, pj, P[iMax]);

    if(freeFlag) free(flag);
    free(centers);

    //among two, return the one that passes the check, and prefer smaller radius
    bool check1 = check<2,circleT>(&circle1, P, n, false);
    if (check1 && circle1.radius() < circle2.radius()) return circle1;
    bool check2 = check<2,circleT>(&circle2, P, n, false);
    if (check2 && circle2.radius() < circle1.radius()) return circle2;

    if (check1 && check2)
      return circle1.radius() < circle2.radius() ? circle1 : circle2;
    else if (check1) return circle1;
    else if (check2) return circle2;
    else {
      cout << "error, no valid circle, abort()" << endl;
      abort();}

  }
}

circle miniDisc2DParallel(point<2>* P, intT n, point<2> pi, point<2> pj, intT* flag=NULL) {
  typedef circle circleT;
  typedef point<2> pointT;

  auto circle = circleT(pi, pj);
  auto process = [&](pointT p) {
    if (!circle.contain(p)) return true;
    else return false;
  };
  auto cleanUp = [&](pointT* A, intT ci) {
    circle = circleT(pi, pj, A[ci]);
    swap(P[0], P[ci]);
    //swap(P[ci % 100 + 100], P[ci]);
  };
  parallel_prefix(P, n, process, cleanUp, false, flag);
  return circle;
}

circle miniDisc2DParallel(point<2>* P, intT n, point<2> pi, intT* flag=NULL) {
  typedef circle circleT;
  typedef point<2> pointT;

  auto circle = circleT(P[0], pi);
  auto process = [&](pointT p) {
    if (!circle.contain(p)) return true;
    else return false;
  };
  auto cleanUp = [&](pointT* A, intT ci) {
    //circle = miniDisc2DSerial(A, ci, pi, A[ci]);
    circle = miniDisc2DParallel(A, ci, pi, A[ci], flag);
    //circle = miniDisc2DParallel2(A, ci, pi, A[ci], flag);
    swap(P[1], P[ci]);
    //swap(P[ci % 80 + 20], P[ci]);
  };
  parallel_prefix(P, n, process, cleanUp, false, flag);
  return circle;
}

circle miniDisc2DBF(point<2>* P, intT n);

circle miniDisc2DParallel(point<2>* P, intT n) {
  typedef circle circleT;
  typedef point<2> pointT;
  intT* flag = newA(intT, n+1);

  auto circle = circleT(P[0], P[1]);
  //auto circle = miniDisc2DBF(P, 10);
  auto process = [&](pointT p) {
    if (!circle.contain(p)) return true;
    else return false;
  };
  auto cleanUp = [&](pointT* A, intT ci) {
    circle = miniDisc2DParallel(A, ci, A[ci], flag);
    swap(P[2], P[ci]);
    //swap(P[ci % 20], P[ci]);
  };
  parallel_prefix(P, n, process, cleanUp, true, flag);
  //parallel_prefix(P+10, n-10, process, cleanUp, true, flag);

  free(flag);
  return circle;
}

circle miniDisc2DBF(point<2>* P, intT n) {
  typedef circle circleT;
  typedef point<2> pointT;

  auto C = newA(circleT, n*n*n);
  par_for (intT i=0; i<n*n*n; ++i) C[i] = circleT();

  auto valid = [&](circleT c)
    {
     for (intT i=0; i<n; ++i)
       if (!c.contain(P[i])) return false;
     return true;
    };

  //check all possible 3 points
  par_for (intT i=0; i<n; ++i) {
    par_for (intT j=i+1; j<n; ++j) {
      par_for (intT k=j+1; k<n; ++k) {
        auto c = circle(P[i], P[j], P[k]);
        if (valid(c))
          C[i*n*n+j*n+k] = c;
      }
    }
  }

  struct getMin {
    circleT* C;
    getMin(circleT* CC): C(CC) {};
    floatT operator() (intT idx) {
      if (C[idx].isEmpty()) return floatMax();
      else return C[idx].radius();
    }
  };

  intT iMin = sequence::minIndex<floatT, intT, getMin>(0, n*n*n, getMin(C));
  free(C);
  return C[iMin];
}

#endif