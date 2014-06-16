/* Copyright (C) 2012,2013 IBM Corp.
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */
/* EncryptedArray.cpp - Data-movement operations on arrays of slots
 */
#include <NTL/ZZ.h>
NTL_CLIENT
#include "EncryptedArray.h"
#include "polyEval.h"

static void buildDigitPolynomial(ZZX& result, long p, long e);


// extractDigits assumes that the slots of *this contains integers mod p^r
// i.e., that only the free terms are nonzero. (If that assumptions does
// not hold then the result will not be a valid ciphertext anymore.)
// 
// It returns in the slots of digits[j] the j'th-lowest gigits from the
// integers in the slots of the input. Namely, the i'th slot of digits[j]
// contains the j'th digit in the p-base expansion of the integer in the
// i'th slot of the *this.
//
// If the shortCut flag is set then digits[j] contains the j'th digits wrt
// mod-p plaintext space and the highest possible level (for all j). Otherwise
// digits[j] still contains the j'th digit in the base-p expansion, but wrt
// mod-p^{r-j} plaintext space, and all the ciphertexts are at the same level.
void extractDigits(vector<Ctxt>& digits, const Ctxt& c, long r, bool shortCut)
{
  if (r<=0 || r>c.getContext().alMod.getR()) // how many digits to extract
    r = c.getContext().alMod.getR();

  // If the digitPoly in the context is not yet initialized, then compute it
  long p = c.getContext().zMStar.getP();
  ZZX& x2p = (ZZX&) c.getContext().modP_digPoly;
  if (IsZero(x2p) && p!=2)
    buildDigitPolynomial(x2p, p, c.getContext().alMod.getR());

  Ctxt tmp(c.getPubKey(), c.getPtxtSpace());
  digits.resize(r, tmp);      // allocate space
  vector<Ctxt> w(r, tmp);
  for (long i=0; i<r; i++) {
    tmp = c;
    for (long j=0; j<i; j++) {
      if (p==2) w[j].square();
      else      polyEval(w[j], x2p, w[j]); // "in spirit" w[j] = w[j]^p
      tmp -= w[j];
      tmp.divideByP();
    }
    w[i] = tmp; // needed in the next round
    if (shortCut) digits[i] = tmp; // digits[i]=i'th lowest digit
  }
  // If not shortCut, copy w into digits
  if (!shortCut) for (long i=0; i<r; i++) digits[i] = w[i];
}



// Compute a degree-p polynomial poly(x) s.t. for any t<e and integr z of the
// form z = z0 + p^t*z1 (with 0<=z0<p), we have poly(z) = z0 (mod p^{t+1}).
//
// We get poly(x) by interpolating a degree-(p-1) polynomial poly'(x)
// s.t. poly'(z0)=z0 - z0^p (mod p^e) for all 0<=z0<p, and then setting
// poly(x) = x^p + poly'(x).
static void buildDigitPolynomial(ZZX& result, long p, long e)
{
  if (p<2 || e<=1) return; // nothing to do
  long p2e = power_long(p,e); // the integer p^e

  // Compute x - x^p (mod p^e), for x=0,1,...,p-1
  vec_long x(INIT_SIZE, p);
  vec_long y(INIT_SIZE, p);
  for (long j=0; j<p; j++) {
    x[j] = j;
    y[j] = j-PowerMod(j,p,p2e);  // x - x^p (mod p^e)
    if (y[j]<0) y[j] += p2e;
  }
  interpolateMod(result, x, y, p, e);
  assert(deg(result)<p); // interpolating p points, should get deg<=p-1
  SetCoeff(result, p);   // return result = x^p + poly'(x)
}