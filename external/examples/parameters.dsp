declare name 		"Dummy";
declare version 	"1.0";
declare author 		"Heu... me...";


process = _ * param1 * param2 * (param3 / 3) * (param4 /4)
with
{
  param1 = button("button");
  param2 = checkbox("checkbox");
  param3 = vslider("slider", 3, 0 , 3, 0.001);
  param4 = nentry("number", 4, 0 , 4, 1);
};
