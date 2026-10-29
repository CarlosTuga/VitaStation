# VitaStation 1.6.12 beta — correção da rota de abertura do Shell

## Comparação com os candidatos anteriores

O utilizador confirmou que os sons regressaram na 1.6.10 e que o arranque ficou mais rápido. A abertura continuou a falhar nas 1.6.10 e 1.6.11. Esses candidatos não foram publicados como correções confirmadas.

Os registos da 1.6.11 mostram ACK seguido da chamada de abertura cerca de 0,65 segundos depois. Assim, nessas tentativas o Shell já avançava após a saída do chamador. Contudo, `launch-name` devolvia identificadores positivos (`0x00090004`, `0x00040006`, `0x00060004`) e o destino não era encontrado em execução cinco segundos depois. O código só tentava a URI se a chamada por nome devolvesse um valor negativo.

## Alteração

Em `plugins/VitaStationBoot/main.c`, o handoff passa diretamente para `sceAppMgrLaunchAppByUri(0x20000, "psgm:play?titleid=...")`, a mesma rota explícita de execução usada no arranque/atalho PS. Deixa de interpretar um identificador positivo da chamada por nome como prova de execução e de impedir por isso a rota URI. O [contrato VitaSDK](https://github.com/vitasdk/vita-headers/blob/master/include/psp2/appmgr.h) documenta 0x20000 para iniciar por URI e o retorno da função ForShell como identificador de aplicação.

Mantém a confirmação exata do pedido, a espera pela saída do chamador, a estabilização de 200 ms e a proteção contra gestos PS pendentes. Regista o retorno da URI e mantém a verificação de execução cinco segundos depois. Não repete automaticamente uma abertura aceite, nem altera diálogos de outras aplicações.

Pacote 1.6.12 beta, APP_VER 01.72, componentes/bridge 1.6.12. Instalar o VPK e reiniciar completamente a consola para carregar o novo plugin. Apenas fechar e voltar a abrir o launcher mantém o plugin anterior na memória.

## Preservado

Áudio, prioridade e PCM da correção confirmada; cache da biblioteca; pesquisa a cada dois arranques; reconstrução manual; melhorias de saída/preparação da 1.6.11; cópia de segurança de plugins/configurações e preservação de plugins anteriores. Não altera clocks, voltagens ou perfis de energia. O pacote de carátulas continua 1.04.

## Verificação e limites

`tests/run_shell_bridge.py` compila e executa a função de produção `process_launch` com um SDK/sistema de ficheiros simulado: verifica ACK com nonce, ausência de chamada enquanto o chamador está ativo, estabilização, URI/flags corretos, registo de erro, sonda de execução sem confundir aceitação com execução e cancelamento do pedido. Passaram também os testes de atalhos/protocolo e a compilação/validação VitaSDK/VPK.

A abertura física na PS Vita 3.65 ainda precisa de confirmação nesta versão. O teste simulado não prova que a aplicação ficou visível na consola. A release pública anterior continua 1.6.9; não se publica este candidato como correção confirmada antes desse teste.
