> Estado atual: **1.6.5 beta**. Ver [revisão completa](../VitaStation/REVISAO_1.6.5_BETA.md) para funções, validação e limitações; secções de versões anteriores são históricas.

> Estado atual: **1.6.4 beta**. Ver [revisão completa](../VitaStation/REVISAO_1.6.4_BETA.md) para funções, validação e limitações; secções de versões anteriores são históricas.

# Validação da distribuição

## Ensaios aprovados no código

- Testes host: biblioteca, navegação, preferências, tratamento de erros, persistência e interfaces simuladas.
- Integração: catálogos TSV/JSON, filtros de região, tarefas/metadata, categorias de definições, navegação e reconstrução manual.
- ISO/PBP: leitura de SFO/ISO9660, identificação PS1, deduplicação e metadados limitados por tamanho.
- Ordenação: instalações mais recentes nas categorias normais; última utilização em Recentes; cache e preferências de cor persistentes.
- Preparação PSP: testes do código de produção com chamadas de consola simuladas, incluindo versões reconhecidas, backups, rollback, marcador de reinício, SFO/caminho de bolha, chamada de cabeçalho, promoção simulada e rejeição de colisões.
- Lançamento: adaptador AppMgr com chamadas simuladas verifica flags, bolha PSP nativa e alternativa URI.
- Carátulas: offsets/índice/CRC, amostras de payload, limites, preservação, commit e rollback do instalador.
- Compilação VitaSDK e validação SFO/PNG/ZIP dos VPKs.

O linker do SDK emitiu o aviso conhecido sobre crtn.o e .note.GNU-stack; o build terminou. Isto é informação de compilação, não uma prova de comportamento na consola.

## Evidência física anterior e leituras

Foi confirmada a existência de bolhas PSP/PS1 registadas fora de ux0:app e de uma instalação Adrenaline 7 original reconhecida. Não são distribuídas a base de dados nem as configurações privadas usadas nessa leitura.

Uma cópia anterior do pacote 1.03 terminou com 20 205/20 205 imagens, zero falhas, em cerca de 9 minutos. Não foi medido o mesmo tempo para a versão 1.04 maior ou para outros cartões.

## Ainda não validado fisicamente nesta versão

Execução efetiva dos novos jogos/bolhas, atualização dos módulos na primeira abertura, combinação com todos os plugins externos, captura PS/Power, supressão de avisos do Shell, todas as tarefas BGDL/auto-instalações e aparência/desempenho finais.

Pré-visualizações host usam comandos de desenho e métricas aproximadas de fonte; não são capturas reais da Vita. Transferência com hash correto não equivale a instalação nem execução.

## Repetir os testes

Ver COMPILAR.md. Os resultados desta preparação verificam os ficheiros copiados e os testes executados na máquina de desenvolvimento; não substituem novos ensaios na consola alvo.

## Validação adicional 1.6.2

`python VitaStation/tests/run_update_maintenance.py` cobre seleção de Releases e do pacote de carátulas, preservação/backup da configuração taiHEN e o código real de instalação de carátulas com ZIP e I/O simulados (cópia, índice, CRC, falta de espaço e preservação). A integração cobre abrir a confirmação de desinstalação, cancelar por X e por O, e a seleção inicial em Cancelar. O teste de cover_pack cobre ausências, recarga e CRC.

Build VitaSDK e validação de maintenance.self dentro do VPK concluídos. Não foram executadas na consola física a descarga HTTPS nem a desinstalação desta versão.

## Validacao 1.6.3

run_safety_backup.py: copia/SHA-256, referencia imutavel, configuracoes repostas, anteriores/atualizados/Adrenaline e plugins manuais preservados, novo plugin registado removido, original ausente recuperado e corrupcao bloqueada. run_game_ui.py: seis acoes do Triangulo, Start e checkbox/confirmacao de desinstalacao, fundo LiveArea com nome arbitrario. Regressao host/integracao e compilacao/validacao VitaSDK aprovadas. Rede e operacoes reais na consola nao foram testadas nesta versao.
