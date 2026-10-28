# Mapeamento da consola para VitaStation

Este levantamento combina as interfaces do VitaSDK instalado com o âmbito pedido para o produto. Não é uma extração do firmware nem um levantamento completo dos serviços privados da consola. Nenhuma ligação de controlo à Vita foi estabelecida. A autorização do utilizador não altera estas limitações técnicas.

Alvo: PS Vita 3.65 com HENkaku/Ensō. A existência de um símbolo no SDK não prova que uma aplicação normal o possa usar, nem que esteja disponível neste firmware. Todas as funções novas exigem validação na consola.

## Ficheiros

- `console-features.json`: inventário funcional e dependências, incluindo perfil PSN e troféus.
- `sdk-api-inventory.csv`: declarações encontradas nos cabeçalhos `psp2` do SDK local. Inventário estático, não exportações verificadas do firmware. Não inclui todas as APIs do kernel ou de plugins.

## Regras de integração

1. Cada controlo deve ler o valor real, verificar os limites, executar a alteração, verificar o resultado e comunicar uma falha sem simular sucesso.
2. Uma chave do registo não equivale automaticamente a uma alteração aplicada pelo serviço correspondente. Não escrever chaves desconhecidas.
3. PSN requer autenticação e serviços disponíveis. Não apresentar um perfil local como sessão PSN, nem uma alteração local como sincronização.
4. Troféus: leitura e progresso; não confundir apresentação com atribuição ou sincronização de troféus.
5. Funções destrutivas exigem escolha explícita na interface; a autorização para desenvolver não solicita formatar ou restaurar a consola agora.
6. Plugins: primeiro arranque, deteção da versão e plugins existentes, origem e hash fixados, cópia de segurança taiHEN, atualização sem duplicados, validação, recuperação e eventual reinício. O VPK não executa scripts de instalação.
7. O utilizador pediu controlos internos: não substituir funções por atalhos ao painel original.

## Estado do código

A versão publicada continua a ser 1.3. Os novos leitores de catálogos, dados deste mapa e partes importadas do motor de pacotes estão em desenvolvimento. Não são apresentados como uma versão totalmente funcional ou como suporte integral à consola.

## Fontes

- [Manual Sony: definições](https://manuals.playstation.net/document/pt/psvita/settings/index.html)
- [Manual Sony: troféus](https://manuals.playstation.net/document/pt/psvita/trophies/index.html)
- [Manual Sony: PSN](https://manuals.playstation.net/document/pt/psvita/psn/index.html)
- [VitaSDK: cabeçalhos](https://github.com/vitasdk/vita-headers)
- [Motor Usagi](https://github.com/coelhomarcus/usagi-pkgj/tree/744541320532b59335771de67399fdeb231f7284)

O manual online indica firmware 3.73 ou posterior: serve de referência funcional e não comprova a compatibilidade de cada opção com 3.65.

Inventário estático: 1657 declarações em 112 cabeçalhos; 95 áreas funcionais específicas.
