/* File: RattleSup/Rattle.c
 * DateTime: Sat Jan 17 17:52:41 2026
 * Last checked in by: starritt
 *
 * SPDX-FileCopyrightText: 2025-2026 Australian Synchrotron
 * SPDX-License-Identifier: LGPL-2.1-only
 *
 * Description
 * rattle - Rate And Time To Limit Estimate
 *
 * The rattle module monitors a single scalar Process Variable (PV) and estimates
 * a) the rate of change with respect to time; and
 * b) how long until the PV value reaches a specified limit/threshold.
 *
 * The rate of change and the current value, i.e. the slope and intersect, are
 * estimated using a least square fit over a specified number of sample points.
 *
 * The time it will take for the PV to reach the specified limit/threshold
 * value is simply: (threshold - current_value)/rate_of_change.
 *
 * A negative time indicates that the limit has already been exceeded.
 * A NaN value indicates a zero rate of change.
 *
 * The number of sample points used is controlled by the inputs. A small number
 * of sample points will result in more responsive but more noisy estimate;
 * a large number of sample points will result in less responsive but smoother
 * estimate.
 *
 * This module provides multiple sets of rate of change and estimated times;
 * this allows for various window sizes and/or thresholds.
 *
 * The input should be linearised if possible/sensible, e.g. if the input is
 * an exponential decay, then the log of the value (using a CALC record)
 * would a more suitable input.
 *
 * Note: if the input, x, is pre-processed through some function, f, then any
 * threshold must also be modifed using the same function f.
 * Likewise the rate of change provided by rattle is  df/dt as opposed to dx/dt.
 *
 * And a bit of calculus:   df(x(t))/dt  = df/dx  *  dx/dt
 * Therefore:               dx/dt = (df/dt) / (df/dx) =  df/dt * dx/df
 *
 * Inputs
 * INPA - DOUBLE  - the PV to be evaluated.
 * INPB - LONG    - the number of elements to be assessed for OUTB/OUTC.
 * INPC - DOUBLE  - primary limit/threshold value.
 * INPD - LONG    - the number of elements to be assessed for OUTD/OUTE.
 * INPE - DOUBLE  - secondary limit/threshold value.
 * INPF - LONG    - the number of elements to be assessed for OUTF/OUTG.
 * INPG - DOUBLE  - tertiary limit/threshold value.
 * INPH - LONG    - the number of elements to be assessed for OUTH/OUTI.
 * INPI - DOUBLE  - quaternary limit/threshold value.
 * INPJ - LONG    - PV.SEVR, or 0 if you don't care.
 *
 * INPL - LONG    - decimation factor.
 *                  if not specified, then the value 1 is used.
 * INPM - LONG    - The maximum number of elements. This should be a constant.
 *                  Use when INPB, INPD,INPF or INPH are PVs.
 * INPR - LONG    - reset: R == 1 to reset, i.e. set internal sample count to 0
 *                         R != 1 no action
 * INPS - DOUBLE  - rate of change scale, e.g. use 60 for /minute rates
 *                  if not specified, then the value 1.0 is used.
 * INPT - DOUBLE  - time estimate, e.g. use 3600 for time estimated to
 *                  converted to hours.
 *                  if not specified, then the value 1.0 is used.
 * Note: while INPS and INPT may often been the same, this is no required
 *
 * Outputs
 * OUTA - LONG    - the number of mesurements available.
 * OUTB - DOUBLE  - rate of change (based on INPB elements)                 - (EGU/s)*INPS.
 * OUTC - DOUBLE  - estimated time to reach limit/threshold (based on INPC) - secs/INPT.
 * OUTD - DOUBLE  - rate of change (based on INPD elements)                 - (EGU/s)*INPS.
 * OUTE - DOUBLE  - estimated time to reach limit/threshold (based on INPE) - secs/INPT.
 * OUTF - DOUBLE  - rate of change (based on INPF elements)                 - (EGU/s)*INPS.
 * OUTG - DOUBLE  - estimated time to reach limit/threshold (based on INPG) - secs/INPT.
 * OUTH - DOUBLE  - rate of change (based on INPH elements)                 - (EGU/s)*INPS.
 * OUTI - DOUBLE  - estimated time to reach limit/threshold (based on INPI) - secs/INPT.
 *
 *
 * Original author: Andrew Starritt
 * Maintained by:   Andrew Starritt
 *
 * Contact details:
 * as-open-source@ansto.gov.au
 * 800 Blackburn Road, Clayton, Victoria 3168, Australia.
 *
 */

#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>

#include <alarm.h>
#include <cantProceed.h>
#include <dbDefs.h>
#include <dbLink.h>
#include <dbStaticLib.h>
#include <epicsExport.h>
#include <epicsTypes.h>
#include <errlog.h>
#include <aSubRecord.h>
#include <menuFtype.h>
#include <recGbl.h>
#include <registryFunction.h>

#define MAX(a, b)             ((a) >= (b) ? (a) : (b))
#define MIN(a, b)             ((a) <= (b) ? (a) : (b))
#define LIMIT(x,low,high)     (MAX(low, MIN(x, high)))
#define ARRAY_LENGTH(xyz)     (sizeof (xyz) / sizeof (xyz [0]))
#define ZERO2ONE(x)           ((x) <= 0 ? 1 : (x))

// Field access macros.
//
#define DOUBLE_FIELD(field)   *(epicsFloat64*)(prec->field)
#define LONG_FIELD(field)     *(epicsInt32*)(prec->field)

// d is the default
//
#define DOUBLE_VALUE(field,d) (prec->ft##field == menuFtypeDOUBLE ? DOUBLE_FIELD(field) : d )
#define LONG_VALUE(field,d)   (prec->ft##field == menuFtypeLONG   ? LONG_FIELD(field)   : d )

// Check verify field type is of the expected type.
// If not, reports error and sets pact to 1, which stops furher processing.
//
#define CHECK_FIELD_TYPE(ftz, fname, kind) {                                   \
   if (prec->ftz != kind) {                                                    \
      errlogPrintf ("rattle: %s.%s incorrect field type. "                     \
                    "The field is %s, expecting %s.\n",                        \
                    prec->name, fname, fypeName(prec->ftz), fypeName(kind));   \
      prec->pact = 1;                                                          \
      return -1;                                                               \
   }                                                                           \
}

// Structure types
//
typedef struct {
   epicsTimeStamp time;
   epicsFloat64 value;
} Sample;

typedef Sample* SamplePtr;


// We will go fixed max size, at least for now.
//
typedef struct {
   Sample* buffer;
   int sampleCount;
   int maximumSamples;    // treat as constant
   int decimateCount;
   epicsFloat64 decimateTotal;
} RecordData;

typedef RecordData* RecordDataPtr;


// -----------------------------------------------------------------------------
// Local utilities
// -----------------------------------------------------------------------------
//
static const char* fypeName (const menuFtype t)
{
   // I can't find a function in EPICS base to do this.
   //
   switch (t) {
      case menuFtypeSTRING: return "STRING";
      case menuFtypeCHAR:   return "CHAR";
      case menuFtypeUCHAR:  return "UCHAR";
      case menuFtypeSHORT:  return "SHORT";
      case menuFtypeUSHORT: return "USHORT";
      case menuFtypeLONG:   return "LONG";
      case menuFtypeULONG:  return "ULONG";
      case menuFtypeINT64:  return "INT64";
      case menuFtypeUINT64: return "UINT64";
      case menuFtypeFLOAT:  return "FLOAT";
      case menuFtypeDOUBLE: return "DOUBLE";
      case menuFtypeENUM:   return "ENUM";
      default:              return "other";
   }
}

// -----------------------------------------------------------------------------
//
static bool isNanOrInfinite (const double x)
{
   return isnan (x) || isinf (x);
}

// -----------------------------------------------------------------------------
// This function calculates the slope/intersect using a least squares fit.
//
static void slopeIntersect (RecordDataPtr pData, const int number,
                            epicsFloat64* slope, epicsFloat64* intersect)
{
   int firstSlot;
   int lastSlot;     // we go inclusive
   int actual_size;
   int i;
   epicsTimeStamp t0;
   epicsFloat64 x_sum;
   epicsFloat64 y_sum;
   epicsFloat64 xx_sum;
   epicsFloat64 xy_sum;
   epicsFloat64 delta;

   firstSlot = pData->sampleCount - number;
   if (firstSlot < 0) firstSlot = 0;
   lastSlot = pData->sampleCount - 1;

   // We need at least 2 sample points to do anything sensible.
   //
   if (number < 1) {
      *slope = 0.0;
      *intersect = 0.0;
      return;
   }

   if (number < 2) {
      *slope = 0.0;
      *intersect = pData->buffer[lastSlot].value;
      return;
   }

   x_sum = 0.0;
   y_sum = 0.0;
   xx_sum = 0.0;
   xy_sum = 0.0;
   actual_size = 0;

   // We calculate all times relative to the last time, so that the intersect
   // is an estimate of value at time now.
   //
   t0 = pData->buffer[lastSlot].time;

   for (i = firstSlot; i <= lastSlot; i++) {
      epicsFloat64 x = epicsTimeDiffInSeconds (&pData->buffer[i].time, &t0);
      epicsFloat64 y = pData->buffer[i].value;
      x_sum  += x;
      y_sum  += y;
      xx_sum += (x * x);
      xy_sum += (x * y);
      actual_size++;
   }

   delta =   (actual_size * xx_sum) - (x_sum * x_sum);
   *slope = ((actual_size * xy_sum) - (x_sum * y_sum))  / delta;
   *intersect =   ((y_sum * xx_sum) - (x_sum * xy_sum)) / delta;
}


// -----------------------------------------------------------------------------
// Record functions
// -----------------------------------------------------------------------------
//
static long RattleInit (aSubRecord * prec)
{
   RecordDataPtr pData;

   prec->dpvt = NULL;

   // Verify that primary field types are as expected.
   //
   CHECK_FIELD_TYPE (fta, "FTA", menuFtypeDOUBLE);
   CHECK_FIELD_TYPE (ftb, "FTB", menuFtypeLONG);
   CHECK_FIELD_TYPE (ftc, "FTC", menuFtypeDOUBLE);

   // We don't insist that the 2nd, 3rd and 4th input are "correct"
   //
   CHECK_FIELD_TYPE (ftl, "FTL", menuFtypeLONG);     // decimate
   CHECK_FIELD_TYPE (ftj, "FTJ", menuFtypeLONG);     // severity
   CHECK_FIELD_TYPE (ftm, "FTM", menuFtypeLONG);     // maximum
   CHECK_FIELD_TYPE (ftr, "FTR", menuFtypeLONG);     // reset
   CHECK_FIELD_TYPE (fts, "FTS", menuFtypeDOUBLE);   // scale(rate)
   CHECK_FIELD_TYPE (ftt, "FTT", menuFtypeDOUBLE);   // time factor

   CHECK_FIELD_TYPE (ftva, "FTVA", menuFtypeLONG);
   CHECK_FIELD_TYPE (ftvb, "FTVB", menuFtypeDOUBLE);
   CHECK_FIELD_TYPE (ftvc, "FTVC", menuFtypeDOUBLE);

   // Note: static sample sizes work here, dynamic sizes are zero.
   //
   const epicsInt32 n1 = LONG_VALUE (b, 0);
   const epicsInt32 n2 = LONG_VALUE (d, 0);
   const epicsInt32 n3 = LONG_VALUE (f, 0);
   const epicsInt32 n4 = LONG_VALUE (h, 0);
   const epicsInt32 n5 = LONG_VALUE (m, 0);

   const epicsInt32 max = MAX(MAX(MAX(n1, n2), MAX(n3, n4)), n5);
   const size_t number = LIMIT (max, 10, 32768);

   printf ("+++ Rattle Init %s [%ld]\n", prec->name, number);

   // Allocate record specific data.
   //
   pData = (RecordDataPtr) callocMustSucceed
           (1, sizeof (RecordData), "Rattle: initialise");

   pData->sampleCount = 0;
   pData->decimateCount = 0;
   pData->decimateTotal = 0.0;

   // Allocate associated buffer.
   //
   pData->buffer = (SamplePtr) callocMustSucceed
                     (number, sizeof (Sample), "Rattle: initialise");
   pData->maximumSamples = number;

   // Store away the private data for this record into the EPICS record.
   //
   prec->dpvt = pData;

   return 0;
}

// -----------------------------------------------------------------------------
//
static long RattleProcess (aSubRecord * prec)
{
   RecordDataPtr pData;

   // Access the private data for this record.
   //
   pData = (RecordDataPtr) prec->dpvt;
   if (pData == NULL) {
      errlogPrintf ("Rattle: (%s) no data \n", prec->name);
      return (-1);
   }

   // Read input values
   // First read the severity - if invalid skip this update.
   //
   const epicsInt32 severity = LONG_VALUE (j, 0);
   if (severity >= epicsSevInvalid) return 0;

   const epicsFloat64 value = DOUBLE_FIELD (a);

   if (isNanOrInfinite (value)) {
      // This is worse than invalid.
      // Once a nan/inf value gets into the history buffer, it's there
      // until the value falls out of the end of the buffer.
      //
      return (-1);
   }

   // Get sample time - cribbed from Beam_Lifetime_Subroutines.c
   //
   epicsTimeStamp time;
   if (prec->inpa.type == PV_LINK) {
      // Use INPA as time reference
      //
      dbGetTimeStamp (&prec->inpa, &time);
   } else {
      // Use current time.
      //
      epicsTimeGetCurrent (&time);
   }

   // We must limit the number of samples to be valid and
   // not exceed the available buffer size.
   // When a number of points not defined as LONG, we go with 2.
   //
   const epicsInt32 numberSamples [4] = {
      LIMIT (LONG_VALUE (b, 2), 2, pData->maximumSamples),
      LIMIT (LONG_VALUE (d, 2), 2, pData->maximumSamples),
      LIMIT (LONG_VALUE (f, 2), 2, pData->maximumSamples),
      LIMIT (LONG_VALUE (h, 2), 2, pData->maximumSamples)
   };

   const epicsFloat64 thresholds [4] = {
      DOUBLE_VALUE (c, 0.0),
      DOUBLE_VALUE (e, 0.0),
      DOUBLE_VALUE (g, 0.0),
      DOUBLE_VALUE (i, 0.0),
   };

   const epicsInt32   decimateFactor     = ZERO2ONE (LONG_VALUE (l, 1));

   const bool doReset = LONG_FIELD (r) == 1;
   const epicsFloat64 rateOfChangeFactor = ZERO2ONE (DOUBLE_VALUE (s, 1.0));
   const epicsFloat64 timeEstimateFactor = ZERO2ONE (DOUBLE_VALUE (t, 1.0));


   // All input read, now lets get on with it....

   if (doReset) {
      pData->sampleCount = 0;
      pData->decimateCount = 0;
      pData->decimateTotal = 0.0;
   }

   if (pData->decimateCount < decimateFactor) {
      pData->decimateCount++;
      pData->decimateTotal += value;
   }

   // Do we have enough to calculate an average?
   //
   if (pData->decimateCount >= decimateFactor) {
      // Yes we do.
      Sample measurement;
      measurement.time = time;
      measurement.value = decimateFactor == 1 ? value :
                          pData->decimateTotal / decimateFactor;

      pData->decimateCount = 0;   //  reset
      pData->decimateTotal = 0.0;

      if (pData->sampleCount < pData->maximumSamples) {
         // Just add to the measurement buffer
         //
         pData->buffer[pData->sampleCount++] = measurement;

      } else {
         // Buffer is full, need to shuffle the data by one measurement.
         //
         const size_t n = (pData->maximumSamples - 1) * sizeof (pData->buffer[0]);
         memmove(&pData->buffer[0], &pData->buffer[1], n);
         pData->buffer[pData->maximumSamples - 1] = measurement;  // last slot
      }
   }

   LONG_FIELD (vala) = pData->sampleCount;

   epicsFloat64 rate [4];
   epicsFloat64 intersect [4];
   epicsFloat64 eta [4];
   int j;

   for (j = 0; j < 4; j++) {
      slopeIntersect (pData, numberSamples[j], &rate[j], &intersect[j]);
      eta[j] = (thresholds[j] - intersect[j]) / rate[j];

      rate[j] *= rateOfChangeFactor;
      eta[j]  /= timeEstimateFactor;
   }

   DOUBLE_FIELD (valb) = rate[0];
   DOUBLE_FIELD (valc) = eta [0];
   DOUBLE_FIELD (vald) = rate[1];
   DOUBLE_FIELD (vale) = eta [1];
   DOUBLE_FIELD (valf) = rate[2];
   DOUBLE_FIELD (valg) = eta [2];
   DOUBLE_FIELD (valh) = rate[3];
   DOUBLE_FIELD (vali) = eta [3];

   return 0;
}

// -----------------------------------------------------------------------------
//
epicsRegisterFunction (RattleInit);
epicsRegisterFunction (RattleProcess);

/* end */

