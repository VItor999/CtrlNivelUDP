function [angle] = outAngle(T)
%UNTITLED2 Summary of this function goes here
angle =0;
if (T <= 0) 
    angle= 50;
elseif (T < 20000) 
    angle = (50+T/400);
elseif (T <30000) 
    angle = 100;
elseif (T < 50000) 
    angle = (100-(T-30000)/250);
elseif (T < 70000) 
    angle = (20 + (T-50000)/1000);
elseif (T < 100000) 
    angle =(40+20*cos((T-70000)*2*pi/10000));
else
    angle = 100;
end
end

