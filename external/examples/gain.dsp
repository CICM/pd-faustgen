import("stdfaust.lib");

process = _ * (gain)
with
{
  gain = vslider("gain [unit:linear]", 0,0,1,0.001);
};
