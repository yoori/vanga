/* 
 * This file is part of the Vanga distribution (https://github.com/yoori/vanga).
 * Vanga is library that implement multinode decision tree constructing algorithm
 * for regression prediction
 *
 * Copyright (c) 2014 Yuri Kuznecov <yuri.kuznecov@gmail.com>.
 * 
 * This program is free software: you can redistribute it and/or modify  
 * it under the terms of the GNU General Public License as published by  
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but 
 * WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU 
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License 
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <cmath>
#include "PredBuffer.hpp"
#include "VecUtils.hpp"

namespace Vanga
{
  inline double
  cubic_minimizer(
    double u,
    double fu,
    double du,
    double v,
    double fv,
    double dv)
  {
    double d = v - u;
    double theta = (fu - fv) * 3 / d + du + dv;
    double p = std::abs(theta);
    double q = std::abs(du);
    double r = std::abs(dv);
    double s = std::max(std::max(p, q), r);
    /* gamma = s*sqrt((theta/s)**2 - (du/s) * (dv/s)) */
    double a = theta / s;
    double gamma = s * sqrt(a * a - (du / s) * (dv / s));
    if(v < u)
    {
      gamma = -gamma;
    }
    p = gamma - du + theta;
    q = gamma - du + gamma + dv;
    r = p / q;

    assert(!std::isnan(u));
    assert(!std::isnan(r));
    assert(!std::isnan(d));

    return u + r * d;
  }

  inline double
  cubic_minimizer2(
    double u,
    double fu,
    double du,
    double v,
    double fv,
    double dv,
    double xmin,
    double xmax)
  {
    double d = v - u;
    double theta = (fu - fv) * 3 / d + du + dv;
    double p = std::abs(theta);
    double q = std::abs(du);
    double r = std::abs(dv);
    double s = std::max(std::max(p, q), r);
    /* gamma = s*sqrt((theta/s)**2 - (du/s) * (dv/s)) */
    double a = theta / s;
    double gamma = s * std::sqrt(std::max(a * a - (du / s) * (dv / s), 0.0));

    if(u < v)
    {
      gamma = -gamma;
    }

    p = gamma - (dv) + theta;
    q = gamma - (dv) + gamma + (du);
    r = p / q;

    if(r < 0. && gamma != 0.)
    {
      return (v) - r * d;
    }
    else if(a < 0)
    {
      return xmax;
    }
    else
    {
      return xmin;
    }
  }

  inline double
  quard_minimizer(
    double u,
    double fu,
    double du,
    double v,
    double fv)
  {
    double a = v - u;
    return u + du / ((fu - fv) / a + du) / 2 * a;
  }

  inline double
  quard_minimizer2(
    double u,
    double du,
    double v,
    double dv)
  {
    double a = u - v;
    return v + dv / (dv - du) * a;
  }

  inline void
  update_trial_interval(
    bool& brackt,
    double& t,
    double& ft,
    double& dt,
    double& x,
    double& fx,
    double& dx,
    double& y,
    double& fy,
    double& dy,
    const double tmin,
    const double tmax)
  {
    int bound;
    bool dsign = signdiff(dt, dx);

    double mc; // minimizer of an interpolated cubic.
    double mq; // minimizer of an interpolated quadratic.
    double newt; // new trial value.

    // Check the input parameters for errors.
    if(brackt)
    {
      //assert(!((t <= std::min(x, y) || std::max(x, y) <= t)));
      //assert(!(0. <= dx * (t - x)));
      assert(tmax >= tmin);
    }

    // Trial value selection.
    if(fx < ft)
    {
      /*
      Case 1: a higher function value.
      The minimum is brackt. If the cubic minimizer is closer
      to x than the quadratic one, the cubic one is taken, else
      the average of the minimizers is taken.
      */
      brackt = true;
      bound = 1;
      mc = cubic_minimizer(x, fx, dx, t, ft, dt);
      mq = quard_minimizer(x, fx, dx, t, ft);
      newt = std::abs(mc - x) < std::abs(mq - x) ? mc : mc + 0.5 * (mq - mc);
    }
    else if(dsign)
    {
      /*
      Case 2: a lower function value and derivatives of
      opposite sign. The minimum is brackt. If the cubic
      minimizer is closer to x than the quadratic (secant) one,
      the cubic one is taken, else the quadratic one is taken.
      */
      brackt = true;
      bound = 0;
      mc = cubic_minimizer(x, fx, dx, t, ft, dt);
      mq = quard_minimizer2(x, dx, t, dt);
      newt = std::abs(mc - t) > std::abs(mq - t) ? mc : mq;
    }
    else if(std::abs(dt) < std::abs(dx))
    {
      /*
      Case 3: a lower function value, derivatives of the
      same sign, and the magnitude of the derivative decreases.
      The cubic minimizer is only used if the cubic tends to
      infinity in the direction of the minimizer or if the minimum
      of the cubic is beyond t. Otherwise the cubic minimizer is
      defined to be either tmin or tmax. The quadratic (secant)
      minimizer is also computed and if the minimum is brackt
      then the the minimizer closest to x is taken, else the one
      farthest away is taken.
      */
      bound = 1;
      mc = cubic_minimizer2(x, fx, dx, t, ft, dt, tmin, tmax);
      mq = quard_minimizer2(x, dx, t, dt);
      if(brackt)
      {
        newt = std::abs(t - mc) < std::abs(t - mq) ? mc : mq;
      }
      else
      {
        newt = std::abs(t - mc) > std::abs(t - mq) ? mc : mq;
      }
    }
    else
    {
      /*
      Case 4: a lower function value, derivatives of the
      same sign, and the magnitude of the derivative does
      not decrease. If the minimum is not brackt, the step
      is either tmin or tmax, else the cubic minimizer is taken.
      */
      bound = 0;
      if(brackt)
      {
        newt = cubic_minimizer(t, ft, dt, y, fy, dy);
      }
      else
      {
        newt = x < t ? tmax : tmin;
      }
    }

    assert(!std::isnan(t));
    assert(!std::isnan(newt));

    /*
    Update the interval of uncertainty. This update does not
    depend on the new step or the case analysis above.
      - Case a: if f(x) < f(t),
        x <- x, y <- t.
      - Case b: if f(t) <= f(x) && f'(t)*f'(x) > 0,
        x <- t, y <- y.
      - Case c: if f(t) <= f(x) && f'(t)*f'(x) < 0,
        x <- t, y <- x.
    */
    if(fx < ft)
    {
      // Case a
      y = t;
      fy = ft;
      dy = dt;
    }
    else
    {
      // Case c
      if(dsign)
      {
        y = x;
        fy = fx;
        dy = dx;
      }

      // Cases b and c
      x = t;
      fx = ft;
      dx = dt;
    }

    assert(!std::isnan(x));
    assert(!std::isnan(fx));
    assert(!std::isnan(dx));

    // Clip the new trial value in [tmin, tmax].
    newt = std::min(newt, tmax);
    newt = std::max(newt, tmin);

    /*
    Redefine the new trial value if it is close to the upper bound
    of the interval.
    */
    if(brackt && bound)
    {
      mq = x + 0.66 * (y - x);
      if(x < y)
      {
        newt = std::max(newt, mq);
      }
      else
      {
        newt = std::min(newt, mq);
      }
    }

    // Return the new trial value.
    t = newt;

    assert(!std::isnan(t));
  }

  template<typename FunType>
  bool
  line_search(
    int n,
    FloatArray& x_vec, // result point (out)
    double& f, // function value (in/out)
    FloatArray& g_vec, // result gradient (in/out)
    double& stp, // step (in/out)
    const double* s, // direction
    const double* xp, // start point
    const FunType& fun // function evaluator
    )
    throw()
  {
    static const double LINE_SEARCH_XTOL = 1.0e-16;
    static const double LINE_SEARCH_FTOL = 1e-4;
    static const double LINE_SEARCH_GTOL = 0.9;
    static const double LINE_SEARCH_MINSTEP = 1e-20;
    static const double LINE_SEARCH_MAXSTEP = 1e+20;
    static const int LINE_SEARCH_MAX_ITERATIONS = 40;

    int count = 0;
    bool brackt;
    double dg;
    double stx, dgx;
    double sty, dgy;
    double fxm, dgxm, fym, dgym, fm, dgm;
    double stmin, stmax;

    double* x = &x_vec[0];
    const double* g = &g_vec[0];

    // Check the input parameters for errors.
    assert(stp > 0.0);

    // Compute the initial gradient in the search direction.
    double dginit = vec_mul(g, s, n);

    // Make sure that s points to a descent direction.
    assert(0 >= dginit);

    // Initialize local variables.
    brackt = false;
    bool stage1 = true;
    double finit = f;
    double dgtest = LINE_SEARCH_FTOL * dginit;
    double width = LINE_SEARCH_MAXSTEP - LINE_SEARCH_MINSTEP;
    double prev_width = 2.0 * width;

    /*
    The variables stx, fx, dgx contain the values of the step,
    function, and directional derivative at the best step.
    The variables sty, fy, dgy contain the value of the step,
    function, and derivative at the other endpoint of
    the interval of uncertainty.
    The variables stp, f, dg contain the values of the step,
    function, and derivative at the current step.
    */
    double fx = finit;
    double fy = finit;
    stx = sty = 0.;
    dgx = dgy = dginit;

    for (;;)
    {
      assert(!std::isnan(stp));

      /*
      Set the minimum and maximum steps to correspond to the
      present interval of uncertainty.
      */
      if(brackt)
      {
        stmin = std::min(stx, sty);
        stmax = std::max(stx, sty);
      }
      else
      {
        stmin = stx;
        stmax = stp + 4.0 * (stp - stx);
      }

      // Clip the step in the range of [stpmin, stpmax].
      stp = std::max(stp, LINE_SEARCH_MINSTEP);
      stp = std::min(stp, LINE_SEARCH_MAXSTEP);

      assert(!std::isnan(stp));

      /*
      If an unusual termination is to occur then let
      stp be the lowest point obtained so far.
      */
      if ((brackt && (
          stp <= stmin || stmax <= stp ||
          LINE_SEARCH_MAX_ITERATIONS <= count + 1)) ||
        (brackt && (stmax - stmin <= LINE_SEARCH_XTOL * stmax)))
      {
        stp = stx;
      }

      assert(!std::isnan(stp));

      // Compute the current value of x: x <- x + stp * s.
      vec_copy(x, xp, n);
      vec_add(x, s, stp, n);

      // Evaluate the function and gradient values.
      f = fun.eval_fun_and_grad(g_vec, x_vec);

      /*
      {
        std::cerr << "line_search: vars = [";
        Vanga::Algs::print(std::cerr, x_vec.begin(), x_vec.end());
        std::cerr << "], grad = [";
        Vanga::Algs::print(std::cerr, g_vec.begin(), g_vec.end());
        std::cerr << "], stp = " << stp <<
          ", f = " << f << ", brackt = " << brackt << std::endl;
      }
      */

      dg = vec_mul(g, s, n);
      double ftest1 = finit + stp * dgtest;

      ++count;

      // Test for errors and convergence.
      //assert(!(brackt && ((stp <= stmin || stmax <= stp))));
      assert(!std::isnan(stp));
      assert(!(stp == LINE_SEARCH_MAXSTEP && f <= ftest1 && dg <= dgtest));
      assert(!(stp == LINE_SEARCH_MINSTEP && (ftest1 < f || dgtest <= dg)));
      assert(!(brackt && (stmax - stmin) <= LINE_SEARCH_XTOL * stmax));

      if(LINE_SEARCH_MAX_ITERATIONS <= count)
      {
        // Maximum number of iteration.
        //std::cerr << "line_search: max iteration, stp = " << stp << std::endl;
        return false;
      }

      if(f <= ftest1 && std::abs(dg) <= LINE_SEARCH_GTOL * (-dginit))
      {
        // The sufficient decrease condition and the directional derivative condition hold.
        return false;
      }

      /*
      In the first stage we seek a step for which the modified
      function has a nonpositive value and nonnegative derivative.
      */
      if(stage1 && f <= ftest1 && std::min(LINE_SEARCH_FTOL, LINE_SEARCH_GTOL) * dginit <= dg)
      {
        stage1 = false;
      }

      /*
      A modified function is used to predict the step only if
      we have not obtained a step for which the modified
      function has a nonpositive function value and nonnegative
      derivative, and if a lower function value has been
      obtained but the decrease is not sufficient.
      */
      if(stage1 && ftest1 < f && f <= fx)
      {
        // Define the modified function and derivative values.
        fm = f - stp * dgtest;
        fxm = fx - stx * dgtest;
        fym = fy - sty * dgtest;
        dgm = dg - dgtest;
        dgxm = dgx - dgtest;
        dgym = dgy - dgtest;

        // Call update_trial_interval() to update the interval of
        // uncertainty and to compute the new step.
        update_trial_interval(
          brackt,
          stp,
          fm,
          dgm,
          stx,
          fxm,
          dgxm,
          sty,
          fym,
          dgym,
          stmin,
          stmax
          );

        fx = fxm + stx * dgtest;
        fy = fym + sty * dgtest;
        dgx = dgxm + dgtest;
        dgy = dgym + dgtest;
      }
      else
      {
        // Call update_trial_interval() to update the interval of
        // uncertainty and to compute the new step.
        update_trial_interval(
          brackt,
          stp,
          f,
          dg,
          stx,
          fx,
          dgx,
          sty,
          fy,
          dgy,
          stmin,
          stmax
          );
      }

      assert(!std::isnan(stx));
      assert(!std::isnan(sty));
      assert(!std::isnan(stp));

      // Force a sufficient decrease in the interval of uncertainty.
      if(brackt)
      {
        if(0.66 * prev_width <= std::abs(sty - stx))
        {
          stp = stx + 0.5 * (sty - stx);
        }

        prev_width = width;
        width = std::abs(sty - stx);
      }

      assert(!std::isnan(stp));
    }

    return true;
  }

  double
  owlqn_x1norm(
    const double* x,
    const int start,
    const int n)
  {
    double norm = 0.;

    for(int i = start; i < n; ++i)
    {
      norm += std::abs(x[i]);
    }

    return norm;
  }

  void
  owlqn_pseudo_gradient(
    double* pg,
    const double* x,
    const double* g,
    const int n,
    const double c,
    const int start,
    const int end)
  {
    // Compute the negative of gradients.
    for(int i = 0; i < start; ++i)
    {
      pg[i] = g[i];
    }

    // Compute the psuedo-gradients.
    for(int i = start; i < end; ++i)
    {
      if(x[i] < 0.)
      {
        // Differentiable.
        pg[i] = g[i] - c;
      }
      else if (0. < x[i])
      {
        // Differentiable.
        pg[i] = g[i] + c;
      }
      else
      {
        pg[i] = g[i] < -c ? g[i] + c : (c < g[i] ? g[i] - c : 0.0);
      }
    }

    for(int i = end; i < n; ++i)
    {
      pg[i] = g[i];
    }
  }

  struct LBFSGIterationData
  {
    double alpha;
    double* s; // [n]
    double* y; // [n]
    double ys; // vec_mul(y, s)
  };

  template<typename IteratorType, typename FunType>
  void
  lbfsg_logloss_min(
    std::vector<double>& yes_res,
    std::vector<double>& no_res,
    const std::vector<VarGroup<IteratorType> >& preds,
    const FunType& fun)
    throw()
  {
    static const double LBFGS_EPSILON = 1e-5;
    static const int LBFGS_M = 6;
    static const int LBFGS_PAST = 0;
    static const double LBFGS_DELTA = 0.;

    const int n = yes_res.size() + 1;
    const int orthantwise_start = 0;
    const int orthantwise_end = n;
    const double orthantwise_c = 0.;
    const int max_iterations = 1000;

    const int m = LBFGS_M;

    LBFSGIterationData* it = 0;

#if defined(USE_SSE) && (defined(__SSE__) || defined(__SSE2__))
    // Round out the number of variables.
    n = round_out_variables(n);
#endif/*defined(USE_SSE)*/

    // Check the input parameters for errors.
    assert(n > 0);

#if defined(USE_SSE) && (defined(__SSE__) || defined(__SSE2__))

    // SSE requirements
    assert(n % 8 == 0);
    assert((uintptr_t)(const void*)x % 16 == 0);

#endif/*defined(USE_SSE)*/

    assert(orthantwise_c >= 0.);

    if(orthantwise_end < 0)
    {
      orthantwise_end = n;
    }

    // Allocate working space.
    Gears::IntrusivePtr<BufferPtr<double> > x_buf = BufferProvider<double>::instance().get();
    x_buf->buf().resize(n);
    double* x = &(x_buf->buf()[0]);

    Gears::IntrusivePtr<BufferPtr<double> > xp_buf = BufferProvider<double>::instance().get();
    xp_buf->buf().resize(n);
    double* prev_x = &(xp_buf->buf()[0]);
    Gears::IntrusivePtr<BufferPtr<double> > g_buf = BufferProvider<double>::instance().get();
    g_buf->buf().resize(n);
    double* g = &(g_buf->buf()[0]);
    Gears::IntrusivePtr<BufferPtr<double> > gp_buf = BufferProvider<double>::instance().get();
    gp_buf->buf().resize(n);
    double* prev_g = &(gp_buf->buf()[0]);
    Gears::IntrusivePtr<BufferPtr<double> > d_buf = BufferProvider<double>::instance().get();
    d_buf->buf().resize(n);
    double* d = &(d_buf->buf()[0]);
    Gears::IntrusivePtr<BufferPtr<double> > w_buf = BufferProvider<double>::instance().get();
    w_buf->buf().resize(n);
    double* w = &(w_buf->buf()[0]);

    // Allocate limited memory storage.
    Gears::IntrusivePtr<BufferPtr<double> > lm_buf = BufferProvider<double>::instance().get();
    lm_buf->buf().resize(n);
    double* lm = &(lm_buf->buf()[0]);

    std::vector<Gears::IntrusivePtr<BufferPtr<double> > > buf_holders;

    Gears::IntrusivePtr<BufferPtr<double> > pg_buf;
    double* pg = 0;

    if(orthantwise_c != 0.)
    {
      // Allocate working space for OW-LQN.
      pg_buf = BufferProvider<double>::instance().get();
      pg_buf->buf().resize(n);
      pg = &(pg_buf->buf()[0]);
    }

    // Initialize the limited memory.
    for(int i = 0; i < m; ++i)
    {
      it = &lm[i];
      it->alpha = 0;
      it->ys = 0;

      Gears::IntrusivePtr<BufferPtr<double> > s_buf = BufferProvider<double>::instance().get();
      buf_holders.push_back(s_buf);
      s_buf->buf().resize(n);
      it->s = &(s_buf->buf()[0]);

      Gears::IntrusivePtr<BufferPtr<double> > y_buf = BufferProvider<double>::instance().get();
      buf_holders.push_back(y_buf);
      y_buf->buf().resize(n);
      it->y = &(y_buf->buf()[0]);
    }

    // Allocate an array for storing previous values of the objective function.
    Gears::IntrusivePtr<BufferPtr<double> > pf_buf;
    double* pf = 0;

    if(LBFGS_PAST > 0)
    {
      pf_buf = BufferProvider<double>::instance().get();
      pf_buf->buf().resize(LBFGS_PAST);
      pf = &(pf_buf->buf()[0]);
    }

    // Evaluate the function value and its gradient.
    double fx = fun.eval_fun_and_grad(g, x);

    if(orthantwise_c != 0.)
    {
      // Compute the L1 norm of the variable and add it to the object value.
      double xnorm = owlqn_x1norm(x, orthantwise_start, orthantwise_end);
      fx += xnorm * orthantwise_c;
      owlqn_pseudo_gradient(
        pg, x, g, n,
        orthantwise_c,
        orthantwise_start,
        orthantwise_end
        );
    }

    // Store the initial value of the objective function.
    if(pf != 0)
    {
      pf[0] = fx;
    }

    // Compute the direction;
    // we assume the initial hessian matrix H_0 as the identity matrix.
    if(orthantwise_c == 0.)
    {
      vec_inv_copy(d, g, n);
    }
    else
    {
      vec_inv_copy(d, pg, n);
    }

    // Make sure that the initial variables are not a minimizer.
    double xnorm = std::max(vec_norm(x, n), 1.0);
    double gnorm;

    if(orthantwise_c == 0.)
    {
      gnorm = vec_norm(g, n);
    }
    else
    {
      gnorm = vec_norm(pg, n);
    }

    if(gnorm / xnorm <= LBFGS_EPSILON)
    {
      return;
    }

    double step = 1.0 / vec_norm(d, n);

    int k = 1;
    int end = 0;

    while(true)
    {
      // Store the current position and gradient vectors.
      vec_copy(prev_x, x, n);
      vec_copy(prev_g, g, n);

      // Search for an optimal step.
      bool ls;

      if(orthantwise_c == 0.)
      {
        ls = line_search(
          n, x, fx, g,
          step,
          d, // direction (gradient inv)
          prev_x, prev_g, w, fun);
      }
      else
      {
        ls = line_search(
          n, x, fx, g,
          step,
          d,
          prev_x, pg, w, fun);

        owlqn_pseudo_gradient(
          pg,
          x,
          g,
          n,
          orthantwise_c,
          orthantwise_start,
          orthantwise_end
          );
      }

      if(ls < 0)
      {
        // Revert to the previous point.
        vec_copy(x, prev_x, n);
        vec_copy(g, prev_g, n);
        return;
      }

      // Compute x and g norms.
      xnorm = vec_norm(x, n);
      gnorm = orthantwise_c == 0. ? vec_norm(g, n) : vec_norm(pg, n);

      /*
        Convergence test.
        The criterion is given by the following formula:
          |g(x)| / \max(1, |x|) < \epsilon
      */
      xnorm = std::max(xnorm, 1.0);

      if(gnorm / xnorm <= LBFGS_EPSILON)
      {
        // Convergence.
        break;
      }

      /*
      Test for stopping criterion.
      The criterion is given by the following formula:
        (f(past_x) - f(x)) / f(x) < \delta
      */
      if(LBFGS_PAST > 0)
      {
        const int past = LBFGS_PAST > 0 ? LBFGS_PAST : 1;

        // We don't test the stopping criterion while k < past.
        if(LBFGS_PAST <= k)
        {
          // Compute the relative improvement from the past.
          double prev_fx = pf[k % past];
          double rate = fx > 0.0 ?
            ((prev_fx - fx) / fx) :
            (prev_fx > 0 ? LBFGS_DELTA + 1.0 : 1.0);

          // The stopping criterion.
          if(rate < LBFGS_DELTA)
          {
            break;
          }
        }

        // Store the current value of the objective function.
        pf[k % past] = fx;
      }

      if(max_iterations != 0 && max_iterations < k + 1)
      {
        // Maximum number of iterations.
        break;
      }

      /*
        Update vectors s and y:
          s_{k+1} = x_{k+1} - x_{k} = \step * d_{k}.
          y_{k+1} = g_{k+1} - g_{k}.
      */
      it = &lm[end];
      vec_sub(it->s, x, prev_x, n);
      vec_sub(it->y, g, prev_g, n);

      /*
        Compute scalars ys and yy:
          ys = y^t \cdot s = 1 / \rho.
          yy = y^t \cdot y.
        Notice that yy is used for scaling the hessian matrix H_0 (Cholesky factor).
      */
      double ys = vec_mul(it->y, it->s, n);
      double yy = vec_mul(it->y, it->y, n);
      it->ys = ys;

      /*
       Recursive formula to compute dir = -(H \cdot g).
       This is described in page 779 of:
       Jorge Nocedal.
       Updating Quasi-Newton Matrices with Limited Storage.
       Mathematics of Computation, Vol. 35, No. 151,
       pp. 773--782, 1980.
      */
      int bound = (m <= k) ? m : k;
      ++k;
      end = (end + 1) % m;

      // Compute the steepest direction.
      if(orthantwise_c == 0.)
      {
        // Compute the negative of gradients.
        vec_inv_copy(d, g, n);
      }
      else
      {
        vec_inv_copy(d, pg, n);
      }

      int j = end;
      for(int i = 0; i < bound; ++i)
      {
        j = (j + m - 1) % m; // if (--j == -1) j = m-1;
        it = &lm[j];
        // \alpha_{j} = \rho_{j} s^{t}_{j} \cdot q_{k+1}.
        it->alpha = vec_mul(it->s, d, n);
        it->alpha /= it->ys;
        // q_{i} = q_{i+1} - \alpha_{i} y_{i}.
        vec_add(d, it->y, -it->alpha, n);
      }

      vec_scale(d, ys / yy, n);

      for(int i = 0; i < bound; ++i)
      {
        it = &lm[j];
        // \beta_{j} = \rho_{j} y^t_{j} \cdot \gamma_{i}.
        double beta = vec_mul(it->y, d, n);
        beta /= it->ys;
        // \gamma_{i+1} = \gamma_{i} + (\alpha_{j} - \beta_{j}) s_{j}.
        vec_add(d, it->s, it->alpha - beta, n);
        j = (j + 1) % m; // if (++j == m) j = 0;
      }

      // Constrain the search direction for orthant-wise updates.
      if(orthantwise_c != 0.)
      {
        for(int i = orthantwise_start; i < orthantwise_end; ++i)
        {
          if(d[i] * pg[i] >= 0)
          {
            d[i] = 0;
          }
        }
      }

      // Now the search direction d is ready. We try step = 1 first.
      step = 1.0;
    }
  }
}
