# Ficheiros para GitHub Releases

Esta pasta local reúne os dois VPKs atuais e um arquivo público de fontes completo, com checksums. Os binários/ZIP são ignorados pelo Git normal; este README e SHA256SUMS.txt podem acompanhar os fontes.

Anexar à Release:

- VitaStation-1.6.0-beta.vpk
- Caratulas-de-loja-1.04.vpk
- VitaStation-1.6.0-public-source.zip (fontes dos dois projetos, documentação e licenças)
- SHA256SUMS.txt

O VPK de carátulas tem cerca de 269 MB, acima do limite de 100 MiB do Git normal. Não forçar a adição desse ficheiro ao histórico. Usar Releases ou um fluxo Git LFS definido separadamente.

O arquivo público é gerado desta árvore limpa. Não reutilizar arquivos de trabalho que possam conter logs com caminhos locais, recibos ou backups privados. Repositório: https://github.com/CarlosTuga/VitaStation. Estes pacotes locais destinam-se a Releases; não são incluídos no histórico Git.
