# VitaStation 1.5.2 beta — revisão HTTPS/carátulas

APP_VER 01.52, TITLE_ID VTST00001.

## Verificação na consola

FTP confirmou APP_VER 01.51 instalada. O diagnostic.log contém falhas repetidas de carátulas com «Problem with the SSL CA cert (path? access rights?)». A mensagem da 1.5.1 não incluía o detalhe do backend; não permite concluir se o erro era na importação do conjunto PEM ou noutra etapa do cliente SSL.

## Comparação com Usagi

Inspecionado coelhomarcus/usagi-pkgj, commit 744541320532b59335771de67399fdeb231f7284. src/curlhttp.cpp desativa SSL_VERIFYPEER e SSL_VERIFYHOST; src/imagefetcher.cpp lê os PNG para memória antes de os descodificar. Usa HexFlow e cache local com alternativa da PlayStation Store. O funcionamento do Usagi não confirma que os certificados do VitaStation estejam a ser importados corretamente.

Fontes: https://github.com/coelhomarcus/usagi-pkgj/blob/744541320532b59335771de67399fdeb231f7284/src/curlhttp.cpp e https://github.com/coelhomarcus/usagi-pkgj/blob/744541320532b59335771de67399fdeb231f7284/src/imagefetcher.cpp.

## Alterações

- Elimina a importação do conjunto PEM por CAINFO_BLOB no dispositivo. Durante a compilação, converte as 118 raízes incluídas para DER. No dispositivo, importa cada certificado diretamente para X509_STORE e associa-o ao SSL_CTX do OpenSSL. Mantém verificação de cadeia e hostname ativa.
- Reutiliza o conjunto de confiança inicializado, com controlo de concorrência e referências do OpenSSL. Caminhos de ficheiros CA do SDK e cache de confiança do libcurl desativados para este adaptador.
- Regista a versão 1.5.2 no arranque, o número de raízes efetivamente importadas, a versão OpenSSL e erros específicos de importação. As mensagens de transferência incluem o código numérico e ERRORBUFFER do libcurl, em vez de apenas a descrição genérica.
- PNGs da biblioteca/loja são lidos para memória antes de vita2d_load_PNG_buffer, após validação de dimensões e limite do tamanho do ficheiro. JPEG mantém o carregador existente.
- Preserva Demos desativadas e exclusão de regiões ASIA/Asian da 1.5.1.

## Instalação e validação

Instalar VitaStation-1.5.2-beta.vpk no VitaShell como atualização. Fechar o VitaStation anterior e abrir novamente. Abrir a loja e usar Quadrado para repetir as carátulas. Se houver erro, o novo registo ux0:data/VitaStation/diagnostic.log permite distinguir a etapa e o código exatos.

Compilação Vita, validação de ZIP/SFO/ícone e testes C passaram. Teste no PC com libcurl/OpenSSL reais importou as 118 raízes via callback SSL_CTX e descarregou PNGs do HexFlow/GitHub e Alive/Archive.org com validação ativa. Este resultado não é execução do binário na Vita. A nova versão ainda requer teste real na consola; não se afirma que o erro relatado tenha sido definitivamente eliminado no dispositivo.

Não é necessário instalar novos plugins. Mantêm-se as funcionalidades e limites das revisões anteriores. A nota do linker do SDK em crtn.o permanece.
