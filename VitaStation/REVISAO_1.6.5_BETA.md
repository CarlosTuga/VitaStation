# VitaStation 1.6.5 beta — correção da abertura de aplicações

A versão 1.6.4 condicionava todas as aberturas ao fim da thread de preparação, incluindo tarefas opcionais de backup, plugins e PSP. Enquanto essa thread estava ocupada, X apenas guardava um pedido e mostrava «A preparar componentes». A abertura também dependia obrigatoriamente da resposta do plugin de Shell.

Aplicações e bolhas já instaladas passam a ter abertura imediata, sem aguardar essa preparação. Quando a preparação está ativa ou o plugin não responde, é usada a chamada nativa de abertura da Vita. Nesse caminho o sistema gere o encerramento/suspensão do launcher; o VitaStation não força uma saída seguida de espera pela thread. O sistema pode mostrar a sua confirmação habitual de encerramento nesta alternativa. Com o plugin carregado e preparação concluída, mantém-se a passagem confirmada ao Shell para evitar essa confirmação.

Apenas a criação de uma nova bolha a partir de ISO/PBP espera pela preparação PSP, com mensagem explícita para tentar novamente. Instalações locais ativas continuam protegidas contra abertura concorrente de jogos. Não é eliminada a preparação nem interrompida à força uma escrita de plugins/backup.

A correção aplica-se também às aplicações originais e à abertura do Apollo. As restantes funções e limites da [1.6.4](REVISAO_1.6.4_BETA.md) mantêm-se. A 1.6.4 pode obter esta versão pelo atualizador; o VPK também pode ser instalado pelo VitaShell.

Validação: teste de regressão mantém a preparação ocupada e confirma que X chama o lançador, sem enfileirar nem forçar saída; verifica também a saída no caminho confirmado pelo Shell. Testes de menus/fundos e compilação/validação VitaSDK aprovados. A abertura real e os diálogos na consola física ainda precisam de validação.
