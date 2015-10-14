#include "KVUnits.h"

NamespaceImp(KVUnits)

////////////////////////////////////////////////////////////////////////////////
// BEGIN_HTML <!--
/* -->
<h2>KVUnits</h2>
<h4>Standard units of length, mass, volume, pressure, energy, and their conversion factors</h4>
<pre><span class="keyword"></span>namespace KVUnits {<br> // lengths<br>const Double_t cm = 1.0;<br>const Double_t um = 1.e-4;<br>const Double_t mm = 1.e-1;<br>const Double_t m = 1.e+2;<br> // masses<br>const Double_t g = 1.0;<br>const Double_t kg = 1.e+3;<br>const Double_t mg = 1.e-3;<br>const Double_t ug = 1.e-6;<br> // pressures<br>const Double_t torr = 1.0;<br>const Double_t atm = 760.;<br>const Double_t Pa = atm/101325.;<br>const Double_t mbar = 100.*Pa;<br>   // volumes<br>const long double cc = 1.0l;<br>const long double litre = 1.e+3l;<br>const long double cl = 10.l*cc;<br>const long double ml = cc;<br>};<br><br></pre>
This is a set of numerical constants used to define and convert units
of length, mass, volume, pressure and energy. The standard unit defined for each
quantity is as follows:<br>
<br>
<div style="margin-left: 40px;">standard mass unit : grammes
(KVUnits::g)<br>
standard length unit : centimetres (KVUnits::cm)<br>
standard pressure unit : torr (KVUnits::torr)<br>
</div>
<br>
The value of the corresponding numerical constant for each of the
standard units is 1. The other available units <br>
are:<br>
<br>
<div style="margin-left: 40px;">mass units: microgramme (KVUnits::ug),
milligramme (KVUnits::mg), kilogramme (KVUnits::kg)<br>
length units: micron/micrometre (KVUnits::um), millimetre
(KVUnits::mm), metre (KVUnits::m)<br>
pressure units: millibar (KVUnits::mbar), pascal (KVUnits::Pa),
atmosphere (KVUnits::atm)<br>
</div>
<h4>Unit conversion</h4>
If x is a quantity expressed in terms of one of the standard units, the
corresponding quantity in terms of a different unit is obtained by
dividing x by the appropriate numerical constant:<br>
<br>
<div style="margin-left: 40px;">x = pressure in torr:&nbsp;
(x/KVUnits::mbar) is pressure in millibar;<br>
x = mass in grammes: (x/KVUnits::mg) is mass in milligrammes;<br>
x = length in centimetres: (x/KVUnits::um) is length in
microns/micrometres.<br>
</div>
<br>
On the other hand, if x is a quantity expressed in arbitrary units,
then in order to express it in terms of standard units multiply x by
the appropriate numerical constant:<br>
<br>
<div style="margin-left: 40px;">x = pressure in atmospheres:
(x*KVUnits::atm) is pressure in torr;<br>
x = mass in microgrammes: (x*KVUnits::ug) is mass in grammes;<br>
x = length in millimetres: (x*KVUnits::mm) is length in centimetres.<br>
</div>
<h4>Composite units</h4>
Similar conversions can be achieved by combinations of numerical
constants. For example, if x is a density expressed in kilogrammes per
cubic metre, the corresponding density in standard units (g/cm**3) is<br>
<br>
<div style="margin-left: 40px;">x*KVUnits::kg/pow(KVUnits::m, 3)<br>
</div><!-- */
// --> END_HTML
////////////////////////////////////////////////////////////////////////////////


