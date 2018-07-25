import("stdfaust.lib");

// Example programmed by Christophe Lebreton - GRAME

f(i) = hslider("freq%3i", 160.,-0.,20000.,0.001);
r(i) = hslider("decay%3i", 0.,0.,1.,0.001):((pow(4.78)*6)+0.0001):ba.tau2pole;
g(i) = hslider("gain%3i", 0.,0.,1.,0.0001);

resonator(n) = _<:par(i,n,*(g(i)):fi.nlf2(f(i),r(i)):_,!:*(ba.db2linear((100*(log(1/r(i))))))):>*(0.003162);

process  = resonator(20) ;
