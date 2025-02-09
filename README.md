Interfaces de comunica��o serial na BitDogLab

Descri��o:

Este projeto implementa o controle de LEDs e a exibi��o de caracteres na matriz de LEDs e no display da BitDogLab. A l�gica do sistema permite a altern�ncia de estados de LEDs e a exibi��o de letras (A-Z, a-z) e n�meros (0-9) tanto no display quanto na matriz de LEDs.

Os estados dos LEDs seguem uma l�gica espec�fica, alternando conforme a entrada dos bot�es, enquanto os caracteres s�o processados e exibidos corretamente nos dispositivos de sa�da.

Componentes Necess�rios:

BitDogLab (com matriz de LEDs, display e bot�es)
Microcontrolador RP2040
Bibliotecas para controle do display e da matriz de LEDs

Funcionamento:

No controle dos LEDs, quando os bot�es s�o pressionados, os LEDs alternam entre os estados definidos.
O estado dos LEDs � atualizado no display, e tamb�m � enviado para o serial monitor.

Na exibi��o de Caracteres, o sistema reconhece e processa caracteres alfanum�ricos (A-Z, a-z, 0-9).
O caractere atual � exibido no display OLED, e para os n�meros tem exibi��o na matriz de LEDs.

Interrup��es e Debouncing:

O c�digo implementa uma l�gica de interrup��o para responder rapidamente � entrada dos bot�es.
Um mecanismo de debouncing evita m�ltiplos acionamentos acidentais.

Estrutura do C�digo:

Inicializa��o: Configura os pinos GPIO para LEDs e bot�es.
Interrup��es: Monitora os bot�es e altera o estado dos LEDs e dos caracteres exibidos.
Renderiza��o: Atualiza o display e a matriz de LEDs com as informa��es atuais.

Autor:
Mateus Moreira da Silva

Este projeto foi desenvolvido e testado utilizando a BitDogLab com o microcontrolador RP2040.
