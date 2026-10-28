# VitaStation 1.5.1 beta

APP_VER 01.51, TITLE_ID VTST00001. Atualização da versão 1.5.

- HTTPS: os certificados de confiança são incorporados no executável e fornecidos ao libcurl em memória com CURLOPT_CAINFO_BLOB. Deixa de passar o caminho app0:assets/cacert.pem para o OpenSSL. Desativa os caminhos CA predefinidos do SDK, mantendo a validação do certificado do servidor e do hostname. Esta alteração trata o erro «Problem with the SSL CA cert (path? access rights?)» relatado no dispositivo.
- Demos desativadas na loja: a categoria desaparece, a navegação por botões e toque usa as oito categorias restantes e o carregamento principal impede a seleção do catálogo Demos. Não apaga TSVs da consola; o parser mantém compatibilidade para testes/formato.
- Entradas com região ASIA/Asian (incluindo variantes AS, ASI, ASN, sem distinção de maiúsculas) são ignoradas nos catálogos TSV e JSON. O mesmo filtro aplica-se à procura automática de DLCs. O filtro usa os metadados de região, não o país do servidor que aloja as imagens.

Instalar VitaStation-1.5.1-beta.vpk no VitaShell como atualização. Abrir a loja e pressionar Quadrado se for necessário tentar novamente as carátulas que falharam. Não é necessário instalar plugins adicionais para esta correção.

Verificações: regressão C, integração dos catálogos reais, exclusão ASIA em TSV/JSON, ciclo das oito categorias e toque sem Demos. No PC, as raízes CA incorporadas validaram HTTPS e PNGs reais tanto do HexFlow/GitHub como do catálogo Alive/Archive.org. Compilação Vita e validação ZIP/SFO/ícone concluídas. O funcionamento final das carátulas na consola ainda precisa de confirmação do utilizador.

Continuam a aplicar-se as funcionalidades e limites de REVISAO_1.5_BETA.md, incluindo manter o VitaStation aberto durante as descargas/instalações. A nota do linker relativa a crtn.o do SDK permanece.

Referência técnica: https://curl.se/libcurl/c/CURLOPT_CAINFO_BLOB.html.
