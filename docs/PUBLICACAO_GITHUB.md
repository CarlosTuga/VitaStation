# Publicar esta preparação

Esta pasta contém os fontes atuais e os pacotes locais. Destino dos fontes: https://github.com/CarlosTuga/VitaStation. Os pacotes binários permanecem separados do histórico Git.

## Fontes

1. Executar python tools/verify_publication.py na raiz.
2. Criar um repositório no GitHub com o nome pretendido.
3. Usar GitHub Desktop ou Git para enviar os fontes desta pasta. Conservar .gitignore, .gitattributes, licenças, avisos, fontes de terceiros e documentação.
4. Antes do envio, verificar a lista de ficheiros. release-assets contém binários ignorados, e backups/dados pessoais não devem ser adicionados à força.

Exemplo de comandos locais, depois de rever os ficheiros:

    git init
    git add .
    git status
    git commit -m "VitaStation 1.6.0 beta: publicação dos fontes"
    git branch -M main

Adicionar depois o endereço real do repositório e fazer o push. Esta preparação não inventa um proprietário/URL remoto.

## Release

Criar uma Release com a versão 1.6.0 beta e anexar os ficheiros indicados em ../release-assets/README.md, incluindo os dois VPKs, fonte pública completa e checksums.

O VPK de carátulas tem cerca de 269 MB e excede o limite de 100 MiB de um ficheiro no Git normal. Por isso, os pacotes ficam locais nesta preparação e são excluídos do Git. Para distribuir ficheiros grandes, usar Releases; Git LFS é uma alternativa se esse for o fluxo escolhido.

Referência oficial: https://docs.github.com/en/repositories/working-with-files/managing-large-files/about-large-files-on-github

Os pacotes binários devem acompanhar as fontes correspondentes e as licenças aplicáveis. Não anunciar todas as definições como funcionais nem todos os caminhos como testados em hardware. O README e FALHAS_E_LIMITACOES.md distinguem estes estados.

## Materiais privados excluídos

Backups da consola, configuração taiHEN pessoal, base de dados real do LiveArea, IP/FTP, recibos de transferência, cache de ferramentas e logs antigos com caminhos de utilizador. Ver EXCLUSOES_DA_PUBLICACAO.json, sem inventário detalhado dos plugins privados.

O logótipo PlayStation e as carátulas mantêm os direitos e restrições indicados em LICENSING.md; colocá-los no GitHub não concede uma licença sobre marcas/imagens de terceiros.
