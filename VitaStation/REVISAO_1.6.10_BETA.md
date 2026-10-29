# VitaStation 1.6.10 beta — restauro do áudio e abertura pelo Shell

Comparação: [1.6.9 → 1.6.10](https://github.com/CarlosTuga/VitaStation/compare/v1.6.9-beta...v1.6.10-beta). Versão nativa do pacote: 01.70; versão apresentada: 1.6.10 beta.

## Áudio mudo na 1.6.9

O utilizador confirmou que o logótipo continuava visível, que a opção de efeitos estava ligada e que todos os sons tinham ficado mudos. `audio.c` repõe a prioridade 0x10000100 da thread de áudio, usada antes da alteração da 1.6.9. A prioridade modificada era uma hipótese de falha do arranque do serviço; o código de erro dessa versão não era registado, portanto não se apresenta a hipótese como diagnóstico físico confirmado.

São acrescentados registos `audio-init`, `audio-port` e `audio-boot` com os resultados reais de criação/arranque do serviço, abertura da porta e configuração/saída do som. Se a reposição do tamanho do bloco dos efeitos falhar depois do boot, a porta é reaberta com a configuração original, em vez de enviar efeitos curtos para uma porta ainda configurada para blocos grandes. Mantém o PCM original, a pré-carga, o padding silencioso e os sons dos botões. Não força alterações nas preferências de som guardadas.

## Aplicações não abriam

A leitura FTP confirmou que o bridge 1.6.9 estava efetivamente carregado e que o último pedido para VITASHELL tinha retorno zero; os registos mostravam novas aberturas do VitaStation após os pedidos. Retorno zero é aceitação da API, não prova de que o destino ficou visível.

Em `plugins/VitaStationBoot/main.c`:

- Verifica a existência do processo chamador diretamente pela Title ID, com consulta adicional da lista de aplicações. Aguarda 200 ms de ausência estável antes de lançar o destino.
- Usa primeiro `sceAppMgrLaunchAppByName2ForShell`, a chamada de abertura do Shell; só tenta a URI se esta falhar. Continua a esperar que o VitaStation termine, sem reintroduzir a abertura comum enquanto o launcher está vivo.
- Suspende o encaminhamento normal PS durante a passagem e nos três segundos seguintes. Descarta gestos PS pendentes e exige observar uma libertação fresca antes de voltar a aceitar um novo toque, para impedir um retorno involuntário ao launcher durante a troca. PS+L mantém a prioridade de saída explícita.
- Regista confirmação, método/retorno da abertura, eventos PS e uma consulta de existência do destino aos cinco segundos em `ux0:data/VitaStation/bridge.log`, limitado a 64 KiB. O resultado imediato continua em `launch-result.bin`.

O novo bridge é identificado como **1.6.10** em `bridge-version.txt`. É necessário instalar o VPK e reiniciar completamente a consola para carregar este módulo. Não basta suspender ou voltar a abrir a aplicação. Não são introduzidos cliques automáticos em diálogos de outras aplicações nem é alegada uma interceção global do SceShell.

## Desempenho preservado

Mantém as otimizações que o utilizador confirmou terem acelerado o VitaStation: cache da biblioteca, pesquisa a cada dois arranques da consola, reconstrução manual e separação da preparação dos plugins. Não altera clocks, voltagens, perfis de energia, frequência da pesquisa ou estrutura da biblioteca.

O marcador de preparação passa a usar `VS_COMPONENTS_VERSION`. Só deve mudar quando os plugins/runtime incluídos mudam; uma atualização apenas de áudio não deve repetir essa preparação. Nesta versão o bridge mudou, por isso a referência dos componentes é 1.6.10. Mantém as correções do Apollo/AutoPlugin, contagem das 21.811 carátulas, instalação automática e backup apenas dos plugins.

## Verificação

Testes host/integração, leitura/padding PCM, pedidos de integração, nonce/lease, ausência estável do chamador, descarte de gesto PS antigo e aceitação de novo gesto passaram. Compilação e validação VitaSDK/VPK aprovadas. As cópias privadas de diagnóstico não são publicadas. A reprodução audível, abertura visual dos destinos e compatibilidade real dos plugins precisam de confirmação depois de instalar/reiniciar; não foram substituídas por alegações de sucesso baseadas só no retorno da API.
