/**
 * The MIT License (MIT)
 *
 * Copyright (c) 2020 Igor Zinken - https://www.igorski.nl
 *
 * Adapted from public source code by alex@smartelectronix.com
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#include "formantfilter.h"
#include "calc.h"
#include <cmath>

namespace Igorski {

/* constructor / destructor */

/**
 * @param aVowel {float} interpolated value within
 *                       the range of the amount specified in the coeffs Array
 */
FormantFilter::FormantFilter( float aVowel, float sampleRate )
{
    memset( _currentCoeffs, 0.0, COEFF_AMOUNT );
    memset( _memory,        0.0, MEMORY_SIZE );

    init_formant();

    _sampleRate = sampleRate;
    setVowel( aVowel );

    lfo = new LFO( _sampleRate );
    hasLFO = false;
}

FormantFilter::~FormantFilter()
{
    delete lfo;
}

/* public methods */

float FormantFilter::getVowel()
{
    return ( float ) _vowel;
}

void FormantFilter::setVowel( float aVowel )
{
    _vowel = ( double ) aVowel;

    double tempRatio = _tempVowel / std::max( 0.000000001, _vowel );

    // in case FormantFilter is attached to oscillator, keep relative offset
    // of currently moving vowel in place
    _tempVowel = ( hasLFO ) ? _vowel * tempRatio : _vowel;

    F = ( int ) Calc::scale( aVowel, 1.f, ( float ) COEFF_AMOUNT - 1);

    if ( hasLFO ) {
        cacheLFO();
    }
}

void FormantFilter::setLFO( float LFORatePercentage, float LFODepth )
{
    bool wasEnabled = hasLFO;
    bool enabled    = LFORatePercentage > 0.f;

    hasLFO = enabled;

    bool hadChange = ( wasEnabled != enabled ) || _lfoDepth != LFODepth;

    if ( enabled )
        lfo->setRate(
            VST::MIN_LFO_RATE() + (
                LFORatePercentage * ( VST::MAX_LFO_RATE() - VST::MIN_LFO_RATE() )
            )
        );

    // turning LFO off
    if ( !hasLFO && wasEnabled ) {
        _tempVowel = _vowel;
        cacheVowel();
    }

    if ( hadChange ) {
        _lfoDepth = LFODepth;
        cacheLFO();
    }
}

void FormantFilter::process( double* inBuffer, int bufferSize )
{
    size_t j, k;
    double res;
    float halfRate = _sampleRate * 0.5f;

    for ( size_t i = 0; i < bufferSize; ++i )
    {
            if (++tmp > (10 *_sampleRate)) tmp = 1; //QQQ

        // when LFO is active, keep it moving

        if ( hasLFO ) {
            float lfoValue = lfo->peek() * .5f  + .5f; // make waveform unipolar
            float scaledValue = std::min( _lfoMax, _lfoMin + _lfoRange * lfoValue ); // relative to depth

            F = ( int ) Calc::scale( scaledValue, 1.f, ( float ) COEFF_AMOUNT - 1);

            f0=12*powf(2.0, 4 - 4 * scaledValue); //sweep
            f0*=(1.0 + 0.01 * sinf( tmp * 0.0015));   //vibrato
            un_f0 = 1.0 / f0;

//            _tempVowel = ;
//            cacheVowel();
        }    else {
            // todo this should not be the bedoeling
            // sound stops when disabling lfo
            f0=12*powf(2.0, 4 - 4 * (tmp/10*_sampleRate)); //sweep
            f0*=(1.0 + 0.01 * sinf( tmp * 0.0015));   //vibrato
            un_f0 = 1.0 / f0;
        }


        dp0 = f0 * (1 / halfRate );
        p0 += dp0;  //phase increment
        p0-= 2 * (p0 > 1);

        { //smoothing of the commands.
          double r = 0.001;
          f1 += r * (F1[F] - f1);
          f2 += r * (F2[F] - f2);
          f3 += r * (F3[F] - f3);
          f4 += r * (F4[F] - f4);
          a1 += r * (A1[F] - a1);
          a2 += r * (A2[F] - a2);
          a3 += r * (A3[F] - a3);
          a4 += r * (A4[F] - a4);
        }

        double inp0 = inBuffer[ i ];

        //The f0/fn coefficients stand for a -3dB/oct spectral envelope

         inBuffer[ i ] =
                        a1 * (f0 / f1 ) * inp0 /*formant(p0, 100 * un_f0) */* porteuse(f1 * un_f0, p0)
                  +/*0.7f**/a2 * (f0 / f2 ) * inp0 /*formant(p0, 120 * un_f0) */* porteuse(f2 * un_f0, p0)
                  +     a3 * (f0 / f3 ) * inp0 /*formant(p0, 150 * un_f0) */* porteuse(f3 * un_f0, p0)
                  +     a4 * (f0 / f4 ) * inp0 /*formant(p0, 300 * un_f0) */* porteuse(f4 * un_f0, p0);

//        res = _currentCoeffs[ 0 ] * inBuffer[ i ];
//
//        for ( j = 1, k = 0; j < COEFF_AMOUNT; ++j, ++k ) {
//            res += _currentCoeffs[ j ] * _memory[ k ];
//        }
//
//        j = MEMORY_SIZE;
//        while ( j-- > 1 ) {
//            _memory[ j ] = _memory[ j - 1 ];
//        }
//
//        //res = ((fabs(res) > 1.0e-10 && res < 1.0) || res < -1.0e-10 && res > -1.0) && !isinf(res) ? res : 0.0f;
//        undenormaliseDouble( res );
//
//        _memory[ 0 ] = res;
//
//        // write output
//
//        inBuffer[ i ] = res;
//
//
//
    }
}

//Formantic function of width I (used to fill the table of formants)
double FormantFilter::fonc_formant(double p,const double I)
{
  double a=0.5f;
  int hmax=int(10*I)>L_TABLE/2?L_TABLE/2:int(10*I);
  double phi=0.0f;
  for(int h=1;h<hmax;h++)
  {
    phi+=3.14159265359f*p;
    double hann=0.5f+0.5f*fast_cos(h*(1.0f/hmax));
    double gaussienne=0.85f*exp(-h*h/(I*I));
    double jupe=0.15f;
    double harmonique=cosf(phi);
    a+=hann*(gaussienne+jupe)*harmonique;
   }
  return a;
}

//Initialisation of the table TF with the fonction fonc_formant.
void FormantFilter::init_formant()
{ double coef=2.0f/(L_TABLE-1);
  for(int I=0;I<I_MAX;I++)
    for(int P=0;P<L_TABLE;P++)
      TF[P+I*L_TABLE]=fonc_formant(-1+P*coef,double(I));
}

//This function emulates the function fonc_formant
// thanks to the table TF. A bilinear interpolation is
// performed
double FormantFilter::formant(double p,double i)
{
  i=i<0?0:i>I_MAX-2?I_MAX-2:i;    // width limitation
    double P=(L_TABLE-1)*(p+1)*0.5f; // phase normalisation
    int P0=(int)P;  double fP=P-P0;  // Integer and fractional
    int I0=(int)i;  double fI=i-I0;  // parts of the phase (p) and width (i).
    int i00=P0+L_TABLE*I0;  int i10=i00+L_TABLE;
    //bilinear interpolation.
    return (1-fI)*(TF[i00] + fP*(TF[i00+1]-TF[i00]))
        +    fI*(TF[i10] + fP*(TF[i10+1]-TF[i10]));
}

// Double carrier.
// h : position (float harmonic number)
// p : phase
double FormantFilter::porteuse(const double h, const double p)
{
  double h0 = floor(h);  //integer and
  double hf = h-h0;      //decimal part of harmonic number.
  // modulos pour ramener p*h0 et p*(h0+1) dans [-1,1]
  double phi0 = fmodf(p* h0   +1+1000, 2.0) - 1.0;
  double phi1 = fmodf(p*(h0+1)+1+1000, 2.0) - 1.0;
  // two carriers.
  double Porteuse0=fast_cos(phi0);
  double Porteuse1=fast_cos(phi1);
  // crossfade between the two carriers.
  return Porteuse0 + hf * (Porteuse1 - Porteuse0);
}

/* private methods */

void FormantFilter::cacheVowel()
{

/*    _tempVowel;

    // vowels are defined in 0 - MAX_VOWEL range
    int MAX_VOWEL = VOWEL_AMOUNT - 1;

    double vowelValue = Calc::scale( _tempVowel, 1.f, ( float ) MAX_VOWEL);

    // interpolate the value between vowels

    int roundVowel  = ( int )( vowelValue );
    double fracpart = ( double ) roundVowel - vowelValue;

    // formants were calculated at 44.1 kHz, scale to match actual sample rate

    double scaleValue = ( double ) _sampleRate / 44100.;

    for ( int i = 0; i < COEFF_AMOUNT; i++ )
    {
        double scaledCoeff = COEFFICIENTS[ roundVowel ][ i ] * scaleValue;

        if ( INTERPOLATE ) {

            // add next vowel (note the overflow check when roundVowel is MAX_VOWEL)
            double nextScaledCoeff = COEFFICIENTS[ roundVowel + ( roundVowel < MAX_VOWEL )][ i ] * vowelValue;
            _currentCoeffs[ i ]    = fracpart * scaledCoeff + ( 1.0 - fracpart ) * nextScaledCoeff;

        } else {
            _currentCoeffs[ i ] = scaledCoeff;
        }
    }
    */
}

void FormantFilter::cacheLFO()
{
    _lfoRange = _vowel * _lfoDepth;
    _lfoMax   = std::min( 1., _vowel + _lfoRange / 2. );
    _lfoMin   = std::max( 0., _vowel - _lfoRange / 2. );
}

}