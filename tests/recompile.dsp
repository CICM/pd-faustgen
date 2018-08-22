declare name 		"Dummy";
declare version 	"1.0";
declare author 		"Heu... me...";


process =_ <: _ * gain1, _ * gain2
with
{
  gain1 = vslider("gain1 [unit:linear]", 0.2, 0 , 1, 0.001);
  gain2 = vslider("gain2 [unit:linear]", 0.2, 0 , 1, 0.001);
};
