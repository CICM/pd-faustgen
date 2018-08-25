declare name 		"Dummy";
declare version 	"1.0";
declare author 		"Heu... me...";


process = _ * gain1 * gain2 * gain3 * gain4
with
{
  gain1 = vslider("gain1 [unit:linear]", 0.1, 0 , 1, 0.001);
  gain2 = vslider("gain2 [unit:linear]", 0.2, 0 , 1, 0.001);
  gain3 = vslider("gain3 [unit:linear]", 0.3, 0 , 1, 0.001);
  gain4 = vslider("gain4 [unit:linear]", 0.4, 0 , 1, 0.001);
};
