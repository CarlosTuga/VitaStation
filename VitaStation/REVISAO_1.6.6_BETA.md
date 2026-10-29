# VitaStation 1.6.6 beta — abertura pelo Shell

Corrige as opções passadas a `sceAppMgrLaunchAppByUri` pelo VitaStationBoot: a abertura direta usa 0x20000; a abertura intencional do LiveArea usa 0. Anteriormente 0xFFFFF podia abrir o LiveArea em vez de executar a aplicação. A correção abrange arranque, PS simples, passagem de aplicações ao Shell e pedido do menu Power. Mantém a alternativa nativa e a correção do bloqueio de preparação da 1.6.5.

Depois da instalação, abrir VitaStation, aguardar a preparação do plugin e reiniciar uma vez. `ux0:data/VitaStation/bridge-version.txt` identifica a versão carregada (1.6.6). O resultado da última passagem ao Shell fica em `launch-result.bin`: nove bytes de ID seguidos de um inteiro de 32 bits com o retorno da API. Estes registos não demonstram que o utilizador viu a aplicação aberta.

## Caso ainda não resolvido

Quando o utilizador abre uma bolha diretamente no LiveArea com VitaStation suspenso, a confirmação de encerramento é do SceShell. Esse pedido não atravessa o código de abertura do VitaStation. Esta versão não interceta nem aceita globalmente esse diálogo. Não clicar automaticamente em OK sem identificar a aplicação afetada, pois isso poderia fechar outra aplicação. O atalho PS+L e a saída do launcher continuam disponíveis, mas não há garantia de encerramento imediato enquanto há preparação/instalação ativa. Não afirmar que a confirmação do LiveArea foi eliminada.

## Diagnóstico e validação

Uma leitura por FTP da consola de teste confirmou APP_VER 01.64 e registos de pesquisa da biblioteca com cerca de 43 segundos. A correção da 1.6.5 ainda não estava instalada nessa leitura. As cópias privadas dos registos/configuração não são incluídas nas fontes públicas; não foram alterados config.txt, firmware ou ficheiros de outras aplicações por FTP.

Compilação/validação VitaSDK, regressão de menus/abertura durante preparação e protocolo/atalhos aprovados. Os resultados reais do novo plugin após reinício e o comportamento visual na consola continuam por confirmar. Limites das versões [1.6.5](REVISAO_1.6.5_BETA.md) e [1.6.4](REVISAO_1.6.4_BETA.md) mantidos.

Contrato da API: https://github.com/vitasdk/vita-headers/blob/master/include/psp2/appmgr.h
