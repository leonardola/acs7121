#include <LiquidCrystal.h> 
#include <TimerOne.h>

LiquidCrystal lcd(12, 11, 5, 4, 3, 13);
const unsigned long tempoLeituras = 100000UL;                                 // valor de 100ms para ler 6 ondas completas no caso de 60Hz
const unsigned long numeroLeituras = 250UL;                                   // numero de leituras que serão vericadas, neste caso o valor é para ler durante 1s
const unsigned long intervaloEntreLeituras = tempoLeituras/numeroLeituras;    // tempo de espera entre cada leitura
int pontoZeroAnalog;                                                          // auto ajuste do ponto zero
float correnteRMS = 0;                                                        // variavel que armazena o valor de corrente
int resolucaoAnalog = 1024;                                                   // resolução da entrada analogica em bits 1024
int calibracao = 1000;                                                        // escala que é mostrada na tela
int tensao = 220;                                                             // tensao da rede
float potencia = 0;                                                           // pontencia medida
float resolucaoSensor = 0.066 + 0.004;                                        // no caso do sensor acs712 x30A sendo de 66mV/A - pequenas variações deste valor para calibração
float valorKWh = 0.536397;                                                    // valor do KWh em tarifa baixa no mes de novembro de 2015 pela Celesc em reias
float consumo = 0;                                                            // valor do consumo de energia em centavos

void setup()
{
  lcd.begin(16, 2);  
  pontoZeroAnalog = determinePontoInicial();                                 // Chama a função que determina qual é o ponto zero da entrada de corrente
  delay(1000);
  lcd.setCursor(0,0);
  lcd.print("                ");
  lcd.setCursor(0,1);
  lcd.print("                ");
  Timer1.initialize(1000000);                                               // set o timer em 1s 
  Timer1.attachInterrupt( lerPotencia );                                    // chama a função a cada um segundo
}

void loop(){
  lerCorrente();
  lcd.setCursor(0,0);
  lcd.print("I:");
  lcd.print(correnteRMS);
  lcd.print("A ");
  lcd.setCursor(8,0);
  lcd.print("P:");
  lcd.print(potencia);
  lcd.print("W    ");
  lcd.setCursor(0,1);
  if(consumo < 100){
    lcd.print("c$:");
    lcd.print(consumo);  
  }else{
    lcd.print("R$:");
    lcd.print(consumo/100);
  }
  lcd.print("    ");
}

int determinePontoInicial(){
  long pontoInicial = 0;
  int j = 500;
  lcd.setCursor(0,0);
  lcd.print("Inicializando");
  lcd.setCursor(0,1);  
  for (int i=0; i<5000; i++) {                                    // ler 5000 vezes para estabilizar o valor
    pontoInicial += analogRead(A0);
    delay(1);
    if(j == i){
      j += 500;
      lcd.print(".");                                             // Imprime um ponto a cada 500 medidas
    }
  }
  pontoInicial /= 5000;
  return int(pontoInicial+1);                                     // soma um no valor inicial para estabilizar o valor
}

float lerCorrente(){                                              // Função que faz a leitura da entrada analógica e converte em corrente
  float correnteTemp = 0;
  for (int i=0; i<10; i++) {                                      // laço para executar mais 10 vezes as medições para assim se conseguir um valor mais estável. Totalizando 1s.
    unsigned long correnteAC = 0;
    unsigned int contador = 0;
    unsigned long tempoMicrosAnterior = micros() - intervaloEntreLeituras;  // coloca o valor do tempo 400 ms atrás para executar imediatamente o primeiro condicional
    while (contador < numeroLeituras){                                      // laço do número de leituras
      if (micros() - tempoMicrosAnterior >= intervaloEntreLeituras){        // condicional que impede a medição de ocorrer antes de 400ms
        int valorLido = analogRead(A0) - pontoZeroAnalog;                   // leitura do valor diretamente da entrada analógica
        correnteAC += (unsigned long)(valorLido * valorLido);               // eleva o valor lido ao quadrado
        ++contador;
        tempoMicrosAnterior += intervaloEntreLeituras;
      }
    }
    float ajuste = calibracao *(resolucaoSensor/resolucaoAnalog);           // calculo que retorna o valor de ajuste do set point do valor da entrada analógica  para Amperé
    correnteTemp += ajuste * sqrt((float)correnteAC/(float)numeroLeituras); // a raiz quadrada dos valores obtidos multiplicado pelo ajuste
  }
  correnteRMS = correnteTemp/10;
  if (correnteRMS < .06) {correnteRMS = 0;}                                 // valor para eliminar ruidos na entrada analógica
  return correnteRMS;
}

void lerPotencia(){
  potencia = correnteRMS*tensao;                                            // calcula a potencia consumida em 1s
  consumo += (valorKWh *100 /3600) * (potencia/100);                        // transforma o valor para centavos depois para KWsegundo e multiplica por KW no instante  
}
