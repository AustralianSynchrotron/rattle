#  <span style='color:#ff5500'>Rate And Time To Limit Estimate</span>

## <span style='color:#ff5500'>Introduction</span>

The Rate And Time To Limit Estimate (rattle) moldule is an aSub record
support module that estimates the rate of change of a value with repect to
time, and using that rate estimate the time it will take for the monitored
value to reach a spectified limit aka threshold value.

The rate of change and the current value, i.e. the slope and intersect, are  estimated using a least square fit over a specified number of sample points.

The time it will take for the PV to reach the specified limit/threshold value
is simply:

    (threshold - current_value)/rate_of_change

A negative time indicates that the limit has already been exceeded.

A NaN value indicates a zero rate of change.

The number of sample points used is controlled by the inputs.
A small number will result in more responsive but more noisy estimate;
a large number of sample points will result in less responsive but
smoother estimate.

This module provides multiple sets of rate of change and estimated times;
this allows for various window sizes and/or thresholds.

The input should be linearised if possible/sensible, e.g. if the input
is an exponential decay, then the log of the value (using a CALC record)
would a more suitable input.

__Note:__  if the input, x, is pre-processed through some function, f, 
then any threshold must also be modifed using the same function f.
Likewise, the rate of change provided by rattle is df/dt as opposed to dx/dt

### <span style='color:#ff5500'>A bit of calculus:</span>

   df(x(t))/dt  = df/dx  *  dx/dt

Therefore:

    dx/dt = (df/dt) / (df/dx) =  df/dt * dx/df


## <span style='color:#ff5500'>aSub record field usage</span>

The comments/documentation within the _RattleSup/Rattle.c_ file is
extensive and should be read.


### <span style='color:#ff5500'>processing</span>

&nbsp;

| processing | function name |
|:-----------|:--------------|
| INAM       | RattleInit    |
| SNAM       | RattleProcess |


### <span style='color:#ff5500'>inputs</span>
&nbsp;

| field |  FTx   | NOx | Comment                                               |
|:------|:-------|:----|:------------------------------------------------------|
| INPA  | DOUBLE | 1   | The PV to be  evaluated/rattled.         |
| INPB  | LONG   | 1   | The number of values to be used assessing rate (B)/estimate(C). |
| INPC  | DOUBLE | 1   | The thresold limit for estimate(C). |
| INPD  | LONG   | 1   | The number of values to be used assessing rate (D)/estimate(E). |
| INPE  | DOUBLE | 1   | The thresold limit for estimate(E). |
| INPF  | LONG   | 1   | The number of values to be used assessing rate (F)/estimate(G). |
| INPG  | DOUBLE | 1   | The thresold limit for estimate(G). |
| INPH  | LONG   | 1   | The number of values to be used assessing rate (H)/estimate(I). |
| INPI  | DOUBLE | 1   | The thresold limit for estimate(I). |
| INPJ  | LONG   | 1   | Input severity: &lt;PV&gt;.SEVR, or 0 if you don't care. |
| INPL  | LONG   | 1   | Input decimation factor - default to 1 |
| INPM  | LONG   | 1   | The maximum number of elements. This should be a constant. |
| INPR  | LONG   | 1   | Reset: R == 1 to reset, R != 1 for no action |
| INPS  | DOUBLE | 1   | The rate of change scale factor, e.g. use 60 for /minute rate |
| INPT  | DOUBLE | 1   | The time estimate scale factor, e.g. use 3600 for an estimate in hours. |

#### Notes

While individual window sizes of used for rate evaluation and subsequent
time to limit estimate calculations may be dynamically controlled using
INPB/D/F/H as PV, the maximum window size is determined at record initialisation.
This is the maximum of inputs B,D,F,H and M.
 
While INPS and INPT may often be the same, this is not required.

When not specified, the IOC will default input values to 0.
For inputs L,S and T, rattle interprets these value as&nbsp;1.

When INPL is greated than 1, Rattle averages L values for form a single
arrgregate value add to the set of elements used to calculate rates and
averages.
This would be usedfull if/when trying to estimate the rate of change over
many hours, days of even weeks for a PV that process once per second that
would other wise require a very large window size.
While this saves memory and processing tiime, its introduces a lag to 
responsiveness.


### <span style='color:#ff5500'>outputs</span>

&nbsp;

| field | FTVx   | NOVx | Comment                                           |
|:------|:-------|:-----|:--------------------------------------------------|
| OUTA  | LONG   | 1    | the number of mesurements available.              |
| OUTB  | DOUBLE | 1    | rate of change (based on INPB elements)           |
| OUTC  | DOUBLE | 1    | estimated time to reach limit/threshold (based on INPB & INPC) |
| OUTD  | DOUBLE | 1    | rate of change (based on INPD elements)           |
| OUTE  | DOUBLE | 1    | estimated time to reach limit/threshold (based on INPD & INPE) |
| OUTF  | DOUBLE | 1    | rate of change (based on INPF elements)           |
| OUTG  | DOUBLE | 1    | estimated time to reach limit/threshold (based on INPF & INPG) |
| OUTH  | DOUBLE | 1    | rate of change (based on INPH elements)           |
| OUTI  | DOUBLE | 1    | estimated time to reach limit/threshold (based on INPH & INPI) |

#### Notes

The output rates of change are:  (EGU/s)*INPS

The estimates times to limit/threshold are:  seconds/INPT

OUTX may point to a record (recommaned the PP qualifier), and/or   
clients may reference the VALx fields.
 

<br>
<font size="-1">Last updated: Sat Apr 12 11:56:03 AEST 2025</font>
<br>
