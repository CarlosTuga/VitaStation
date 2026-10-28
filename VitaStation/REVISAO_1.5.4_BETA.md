# VitaStation 1.5.4 beta — carátulas locais permanentes

APP_VER 01.54, TITLE_ID VTST00001. Compilado a pedido do utilizador antes de concluir o pacote independente de imagens.

## Alterações desta versão

- A loja usa apenas carátulas locais nesta beta. Não inicia pedidos de imagens à Internet nem a atualização automática do catálogo Alive. Usa os catálogos TSV/JSON locais e o catálogo Homebrew incluído.
- Prioridade à pasta permanente ux0:data/VitaStation/covers/. Mantém a leitura das pastas de imagens locais configuradas para os catálogos Usagi/PKGj.
- Aceita PNG e JPEG no diretório permanente. Para Homebrews, tenta a chave exata do catálogo e uma chave estável por aplicação, permitindo manter a imagem mesmo que a URL do ícone seja alterada.
- Reiniciar a consola, reabrir a aplicação, atualizar a grelha com Quadrado ou limpar texturas em RAM não remove estes ficheiros. Não foi introduzida expiração nem rotina de limpeza das carátulas permanentes.
- Mensagem na loja identifica «Carátulas locais permanentes». Imagens não instaladas permanecem com marcador; não provocam nova tentativa HTTPS nesta beta.
- Demos continuam desativadas e entradas ASIA/Asian continuam excluídas da loja.

## O que acompanha este VPK

Este é o launcher atualizado. Não contém as cerca de 20 mil imagens da recolha. O VPK separado «Carátulas de loja» ficou para uma próxima etapa por instrução do utilizador. As imagens descarregadas foram guardadas em D:\Vita sdk\VitaStation-cover-work\images, com registos de origem e falhas na pasta superior.

Até instalar esse pacote, a loja mostra as imagens já disponíveis localmente. A recolha terminou com 17.799 imagens HexFlow e 1.145 ícones Alive válidos; cinco ícones Alive falharam. Foram criados também 1.145 aliases estáveis. A fonte alternativa PlayStation Store não devolveu imagens válidas para as 2.430 tentativas de títulos em falta. Não equivale a uma carátula disponível para todos os títulos possíveis.

O instalador independente já compila e tem testes de cópia, CRC, preservação de ficheiros anteriores e recuperação; ainda não foi empacotado com as imagens nem testado na Vita.

## Instalação e limites

Instalar VitaStation-1.5.4-beta.vpk pelo VitaShell como atualização. Fechar a versão anterior e abrir novamente.

A alteração evita o HTTPS para carátulas; não afirma corrigir os certificados das descargas de aplicações. As funções de instalação, serviços de sistema e outras limitações das revisões anteriores mantêm-se. O pacote separado de carátulas e o teste físico desta revisão ficam pendentes.

Compilação Vita, integridade do VPK, regressão e testes de catálogo/categorias passaram. Testada no PC a prioridade da imagem local estável do Homebrew e a manutenção dessa referência após invalidar o cache em RAM. A nota do linker do SDK em crtn.o permanece.
