%% Malha aberta sem atuação e com perturbação
T= linspace(0,150000,15000);
MAX=100;
delta= zeros(1,15000);
influx= zeros(1,15000);
outflux = zeros(1,15000);
level = zeros(1,15000);
level(1) = 0.4;
inangle = zeros(1,15000);
inangle(1)= 50;
dT=10;
for i=2:15000
    if delta(i-1) > 0 
        if delta(i-1) < 0.01*dT 
            inangle(i) = inangle(i-1)+delta(i-1);
            delta(i) = 0;
        else
            inangle(i) = inangle(i-1)+0.01*dT;
            delta(i) = delta(i-1) - 0.01*dT;
        end
    else
        if delta(i-1) < 0 
            if delta(i-1) > -0.01*dT 
                inangle(i) = inangle(i-1)+delta(i-1);
                delta(i) = 0;
            else
                inangle(i) = inangle(i-1)-0.01*dT;
                delta(i) = delta(i-1)+ 0.01*dT;
            end
        end
    end
    if delta(i-1) == 0
         inangle(i) = inangle(i-1);
    end
    ot(i)=outAngle(T(i));
    influx(i) = 1*sin(pi/2*inangle(i)/100);
    outflux(i) = (MAX/100)*(level(i-1)/1.25+0.2)*sin(pi/2*ot(i)/100);
    level(i) = level(i-1)+0.00002*dT*(influx(i)-outflux(i));
    if (level(i)<0)
        level(i)=0;
    elseif (level(i)>1) 
        level(i)=1;
    end
   
end
%% Figura Malha Aberta
figure(1);
hold on
plot(T/1000,inangle,'color',[0.4660 0.6740 0.1880],'LineWidth',3);
plot(T/1000,ot,'color',[0 0.4470 0.7410],'LineWidth',3);
plot(T/1000,level*100,'color',[0.9290 0.6940 0.1250],'LineWidth',3);
grid on 
axis([0 150 0 130])
lgd1 = legend('Válvula de Entrada', 'Perturbação','Nível');
lgd1.FontSize = 20;
alldatacursors = findall(gcf, 'type', 'hggroup', '-property', 'FontSize');
set(alldatacursors,'FontSize',26);
set(alldatacursors,'FontName','Times');
set(alldatacursors, 'FontWeight', 'bold');
xlabel('Tempo (s)','FontSize',30)
ylabel('Amplitude (%)','FontSize',30)
title('Resposta em malha aberta','FontSize',32)
set(gca,'XMinorGrid','on');
set(gca,'YMinorGrid','on')
%% Ensaio MA sem Perturbação
%iniciar abrindo a válvula em 50
value =50;
delta(1) = delta(1) + value;
for i=2:15000
    r =mod(i,1000); 
    if (r==0)
        delta(i-1) =delta(i-2) + acionamento(i);
    end
    if delta(i-1) > 0 
        if delta(i-1) < 0.01*dT 
            inangle(i) = inangle(i-1)+delta(i-1);
            delta(i) = 0;
        else
            inangle(i) = inangle(i-1)+0.01*dT;
            delta(i) = delta(i-1) - 0.01*dT;
        end
    else
        if delta(i-1) < 0 
            if delta(i-1) > -0.01*dT 
                inangle(i) = inangle(i-1)+delta(i-1);
                delta(i) = 0;
            else
                inangle(i) = inangle(i-1)-0.01*dT;
                delta(i) = delta(i-1)+ 0.01*dT;
            end
        end
    end
    if delta(i-1) == 0
         inangle(i) = inangle(i-1);
    end
    ot(i)=100;
    influx(i) = 1*sin(pi/2*inangle(i)/100);
    outflux(i) = (MAX/100)*(level(i-1)/1.25+0.2)*sin(pi/2*ot(i)/100);
    level(i) = level(i-1)+0.00002*dT*(influx(i)-outflux(i));
    if (level(i)<0)
        level(i)=0;
    elseif (level(i)>1) 
        level(i)=1;
    end
end
Tensaio = T;
levelEnsaio = 100*level;
inAngleEnsaio = inangle;
%% Figura Ensaio
figure(2);
hold on
plot(T/1000,inangle,'color',[0.4660 0.6740 0.1880],'LineWidth',3);
plot(T/1000,level*100,'color',[0.9290 0.6940 0.1250],'LineWidth',3);
grid on 
axis([0 150 0 130])
lgd1 = legend('Válvula de Entrada','Nível');
lgd1.FontSize = 20;
alldatacursors = findall(gcf, 'type', 'hggroup', '-property', 'FontSize');
set(alldatacursors,'FontSize',26);
set(alldatacursors,'FontName','Times');
set(alldatacursors, 'FontWeight', 'bold');
xlabel('Tempo (s)','FontSize',30)
ylabel('Amplitude (%)','FontSize',30)
title('Ensaio em malha aberta','FontSize',32)
set(gca,'XMinorGrid','on');
set(gca,'YMinorGrid','on');
%% Dados de validação
T= linspace(0,150000,15000);
MAX=100;
delta= zeros(1,15000);
influx= zeros(1,15000);
outflux = zeros(1,15000);
level = zeros(1,15000);
level(1) = 0.4;
inangle = zeros(1,15000);
inangle(1)= 50;
dT=10;
value =50;
delta(1) = delta(1) + value;

for i=2:15000
    r =mod(i,5000); 
    if (r==0)
        if(i>5000)
            delta(i-1) =delta(i-2) + 50;
        else 
            delta(i-1) =delta(i-2) - 50;
        end
    end
    if delta(i-1) > 0 
        if delta(i-1) < 0.01*dT 
            inangle(i) = inangle(i-1)+delta(i-1);
            delta(i) = 0;
        else
            inangle(i) = inangle(i-1)+0.01*dT;
            delta(i) = delta(i-1) - 0.01*dT;
        end
    else
        if delta(i-1) < 0 
            if delta(i-1) > -0.01*dT 
                inangle(i) = inangle(i-1)+delta(i-1);
                delta(i) = 0;
            else
                inangle(i) = inangle(i-1)-0.01*dT;
                delta(i) = delta(i-1)+ 0.01*dT;
            end
        end
    end
    if delta(i-1) == 0
         inangle(i) = inangle(i-1);
    end
    ot(i)=100;
    influx(i) = 1*sin(pi/2*inangle(i)/100);
    outflux(i) = (MAX/100)*(level(i-1)/1.25+0.2)*sin(pi/2*ot(i)/100);
    level(i) = level(i-1)+0.00002*dT*(influx(i)-outflux(i));
    if (level(i)<0)
        level(i)=0;
    elseif (level(i)>1) 
        level(i)=1;
    end
end
nivelValida = level*100;
inAngleValida = inangle;
%% Figura Validação
figure(3);
hold on
plot(T/1000,inangle,'color',[0.4660 0.6740 0.1880],'LineWidth',3);
plot(T/1000,level*100,'color',[0.9290 0.6940 0.1250],'LineWidth',3);
grid on 
axis([0 150 0 130])
lgd1 = legend('Válvula de Entrada','Nível');
lgd1.FontSize = 20;
alldatacursors = findall(gcf, 'type', 'hggroup', '-property', 'FontSize');
set(alldatacursors,'FontSize',26);
set(alldatacursors,'FontName','Times');
set(alldatacursors, 'FontWeight', 'bold');
xlabel('Tempo (s)','FontSize',30)
ylabel('Amplitude (%)','FontSize',30)
title('Ensaio em malha aberta','FontSize',32)
set(gca,'XMinorGrid','on');
set(gca,'YMinorGrid','on');