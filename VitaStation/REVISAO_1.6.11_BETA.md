# VitaStation 1.6.11 beta — abertura sem esperar pela pesquisa/preparação repetida

Comparação com a release pública anterior: [1.6.9 → 1.6.11](https://github.com/CarlosTuga/VitaStation/compare/v1.6.9-beta...v1.6.11-beta). Inclui o áudio corrigido do candidato 1.6.10; o utilizador confirmou que os sons voltaram a funcionar. Aquele candidato ficou em rascunho porque a abertura ainda apresentava atrasos/falhas.

## Problema observado

O registo da consola mostrou um pedido de abertura que esperou cerca de 77 segundos pela pesquisa da biblioteca e preparação dos componentes. O trabalho chegava a terminar, mas `prepared-1.6.10.done` não existia: o marcador era guardado pela interface depois de receber o resultado, e a saída do launcher podia impedir esse passo. O contador/assinatura da biblioteca tinham o mesmo problema, causando pesquisas/preparações repetidas. «Pedido de abertura enviado» significava apenas confirmação do pedido pelo plugin.

## Correções e ficheiros alterados

- `bootstrap.c` passa a guardar a conclusão no próprio serviço, antes de este terminar. O contador, a assinatura de instalação e a limpeza do pedido de reconstrução são confirmados pelo produtor depois de guardar a cache, sem depender de outra passagem pela interface.
- `bootstrap_state.c` publica o marcador de componentes com ficheiro temporário e preservação do marcador anterior. Uma conclusão válida é idempotente.
- `bootstrap_exit.h` e `main.c` distinguem pesquisa/caches próprias de preparação que pode escrever plugins. Ao abrir uma aplicação, o launcher pode sair sem aguardar a pesquisa. A transição protegida pelo mutex impede que essa thread comece depois a escrever plugins durante a saída. Se uma preparação já começou, a interface continua a renderizar enquanto ela termina em segurança, em vez de bloquear num join.
- `plugins_readiness.inc` verifica em leitura os módulos/configurações necessários. Quando estão atuais e o backup já existe, não volta a calcular os hashes de todos os ficheiros do backup nem altera plugins. Continua a verificar o próprio bridge e os módulos da pasta VitaStation, preserva escolhas desativadas e plugins externos preexistentes. Se uma alteração for necessária, mantém o caminho completo protegido pelo backup.
- `psp_bubbles.cpp` deixa de verificar o mesmo backup duas vezes na preparação e só o exige no caminho que pode modificar o runtime. Módulos já atuais continuam a ser verificados pelos CRCs, sem repetir o backup.

A verificação de leitura foi executada também com cópias privadas dos ficheiros reais da consola, usando a função de produção e SHA-256; indicou componentes atuais, sem escritas necessárias. Essas cópias não são incluídas na publicação.

## Áudio, plugin e velocidade

Não altera o áudio confirmado na 1.6.10: mesma prioridade compatível, mesmo PCM e buffers. Mantém a biblioteca em cache, a pesquisa a cada dois arranques e a reconstrução manual. Não altera clocks, voltagens ou perfis de energia.

O bridge permanece **1.6.10**: não há alteração ao protocolo nem necessidade de o substituir outra vez se essa versão já estiver carregada. A versão do pacote é 1.6.11, APP_VER 01.71. Instalar o VPK e voltar a abrir; só é necessário reiniciar a consola se ainda estiver carregado um plugin anterior ao 1.6.10.

A abertura continua a ser feita pelo Shell depois da saída do chamador, com o intervalo de estabilização e proteção contra gestos PS pendentes. Não volta à chamada que apresentava o quadrado de confirmação enquanto VitaStation estava ativo. Não é alegada uma interceção global de diálogos de aplicações abertas diretamente no LiveArea.

## Verificação

Passaram testes das transições de saída (pesquisa pode terminar com o processo; preparação iniciada deve terminar; saída impede começar uma preparação), publicação do marcador sem polling da interface, verificação de leitura sem escritas, módulos em falta/alterados, caminhos normalizados, escolhas desativadas e atualizações externas preservadas. Passaram regressões host/integração, testes da segurança de backup/restauração e compilação/validação VitaSDK/VPK.

O som da 1.6.10 foi confirmado pelo utilizador. A abertura completa com este novo ciclo de saída ainda requer confirmação na consola após instalar a 1.6.11. Não se considera que um retorno zero da API prove que o destino ficou visível.
