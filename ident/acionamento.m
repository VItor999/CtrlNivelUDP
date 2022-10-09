function [valor] = acionamento(i)
%UNTITLED4 Summary of this function goes here
%   Detailed explanation goes here
    valor = 0;
    tp=1000;
    switch i
        case 1000
            valor =-50; %retorna a 50
        case 2000
            valor =+20; % 70
        case 3000
            valor =-15; % 55
       case 4000
            valor =-50; % 5
       case 5000
            valor=  20; %25
       case 6000
            valor=  30; %55
       case 7000
             valor=  -10; %45
       case 8000
            valor=  +5; %50
       case 9000
             valor=  -30; %20
       case 10000
            valor=  +80; %100
       case 11000
             valor=  -40; %60
       case 12000
             valor=  -27; %33
       case 13000
            valor = 17; %50
       case 14000
            valor = 12; %62
        otherwise
          valor =0;
    end
end

