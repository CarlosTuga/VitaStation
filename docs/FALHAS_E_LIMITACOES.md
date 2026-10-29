> Estado atual: **1.6.4 beta**. Ver [revisão completa](../VitaStation/REVISAO_1.6.4_BETA.md) para funções, validação e limitações; secções de versões anteriores são históricas.

# Falhas, limitações e validação pendente

Estado de referência: VitaStation 1.6.3 beta e Carátulas de loja 1.04. Os relatórios antigos nas subpastas são históricos; não prevalecem sobre este estado.

## Funções realmente indisponíveis

As 78 entradas de consola dividem-se em 2 controlos de volume/brilho, 12 consultas apenas de leitura, 1 ação de suspensão e 63 entradas sem adaptador. Não se afirma que as 63 foram corrigidas ou implementadas.

Exemplos sem integração editável completa: pesquisa/edição Wi-Fi, perfis/autenticação, proxy/DNS editáveis, modo de avião, rede móvel, perfil/avatar/estado online PSN, sessões/privacidade, sincronização de dados e troféus, Bluetooth/emparelhamento, várias opções de acessibilidade, segurança, restauro e serviços Sony.

O menu conserva a indicação de indisponibilidade. Uma página de consulta de rede, relógio ou idioma não permite necessariamente editar o valor. O mapa por opção está em ../VitaStation/mapping/AUDITORIA_CONSOLA_1.6.0.csv. O diagnóstico na consola grava ux0:data/VitaStation/console-settings-audit.tsv e não executa alterações nem ações de energia.

## PSP/PS1 e módulos

- A leitura e o arranque foram implementados; a execução real dos novos caminhos nesta beta ainda não foi validada fisicamente.
- A preparação automática reconhece o Adrenaline 7 original pelo conjunto de módulos. Versões desconhecidas/misturadas são preservadas e recusadas; não há atualização universal de forks.
- O Adrenaline tem de estar instalado e inicializado. Firmware Sony não é distribuído.
- É necessário reinício depois da primeira preparação. O marcador usa uma referência temporal para distinguir sessões; isto não constitui prova de compatibilidade do kernel efetivamente carregado em qualquer configuração externa.
- A troca de módulos é acompanhada por staging, hashes, backups e rollback de renomeações. Corte de energia/falha de armazenamento durante a publicação pode exigir recuperação manual dos backups.
- ISO de PSP e EBOOT.PBP de PSP/PS1 têm leitor. BIN/CUE de PS1 e CSO/DAX/ZSO não têm leitor nesta versão.
- A biblioteca está limitada a 256 entradas. Metadados ausentes, pastas fora dos caminhos suportados ou formatos inesperados podem impedir identificação.
- O ISO não é convertido em executável Vita: é criado um atalho para o conteúdo original. Mover/eliminar esse conteúdo quebra a referência da bolha.

## Lançamento, PS, Power e arranque

O adaptador tenta a rota direta por nome com flags de saída/LiveArea e depois URI validada. Uma resposta aceite não confirma que o jogo arrancou. Os avisos do sistema, a captura global PS/Power e os gestos ainda precisam de validação física.

A ponte de Shell usa rotas URI cujo comportamento depende do sistema. Não injeta OK globalmente nem força o fecho de outras aplicações. LiveArea continua a ser o shell real. Conflitos com outro plugin de arranque podem bloquear a preparação. As opções de arranque e PS dependem da ponte e dos marcadores correspondentes.

## Descargas e instalação

- Uma tarefa LiveArea aceite pode depois falhar, ficar sem espaço ou não instalar por requisitos do conteúdo. O ID da tarefa não prova conclusão.
- VPK/ZIP e certos conteúdos Homebrew são operações locais: precisam do launcher aberto. Não existe daemon universal que continue qualquer instalação depois de o fechar.
- Uma transação de instalação já iniciada não é cancelada a meio. Abrir outra aplicação é bloqueado durante a instalação local ativa.
- DLCs dependem da ID exata, catálogo, requisitos e limite da fila do sistema; não há promessa de todos os DLCs existentes.
- Algumas ligações Homebrew são informativas ou usam instaladores não suportados. Não existe paridade com todos os mecanismos externos.

## Imagens, HTTPS e armazenamento

Cinco coletâneas PS1 Europa/EUA continuam sem imagem confirmada no pacote: NPEF00153, NPEF00154, NPEF00155, NPEF00156 e NPEF00200. Os relatórios identificam ainda quatro ícones Homebrew indisponíveis nas fontes consultadas. Novos catálogos podem conter mais IDs sem imagem.

Foram reportados erros de caminho CA/callback TLS. A revisão reduziu o conjunto de raízes e manteve a verificação de cadeia/hostname; uma análise anterior encontrou também falha de alocação no contexto SSL. As carátulas locais evitam esse caminho HTTPS. Isto não confirma a resolução de todos os downloads HTTPS de aplicações em todas as consolas.

O pacote 1.04 copia um bloco grande. Necessita de espaço para staging e pode conservar o bloco anterior. Depois de cancelar, recomeça a cópia em bloco. Não existe garantia de recuperação automática de todos os cortes de energia durante as renomeações finais. Backups não devem ser apagados antes de confirmar a integridade da instalação.

## Erros históricos com correção no projeto

- Ícone VPK RGBA associado ao erro 0x8010113D: passou a PNG de paleta, 128×128, sem entrelaçamento, validado no pacote.
- Instalação lenta de milhares de imagens: substituída por payload/copiar em bloco, com manifesto/indexação.
- PS1/PSM sem carátulas: identificadas imagens PSM ausentes e diferença entre IDs PSN e seriais físicos PS1; pacote ampliado com ícones e aliases confirmados.
- Biblioteca sem bolhas PSP fora de ux0:app: acrescentada leitura do registo LiveArea e dos conteúdos PSP/GAME.
- Arranque com pesquisa completa em cada abertura: acrescentada base validada, contador e observação de alterações.

Estas correções não equivalem a afirmar ausência de regressões nesta beta. A cobertura dos testes e o que não foi ensaiado fisicamente estão em TESTES.md.

## Atualizador e desinstalação 1.6.2

A recuperação das carátulas requer abrir a opção de atualização e manter o launcher aberto; exige cerca de 600 MiB livres e HTTPS funcional. Não ignora erros de certificado. A presença do bloco instalado é verificada pelo índice/tamanho e leitura de uma imagem, sem auditoria integral de todas as imagens existentes. A instalação do próprio launcher termina no VitaShell.

A remoção executa-se em maintenance.self e usa o gestor de pacotes nativo. Ainda não foi validada fisicamente a remoção da própria aplicação neste contexto. Se o sistema recusar, é necessário remover a bolha pelo LiveArea/VitaShell; o arranque e os atalhos são desativados antes. Uma falha pode deixar remoção parcial. Preferências, backups de recuperação e outros plugins são preservados. Ver REVISAO_1.6.2_BETA.md.

## Funcoes 1.6.3

Backup criado no primeiro arranque antes das alteracoes, sem reconstruir historico de instalacoes antigas. Limpeza adicional opcional: repoe configuracoes e remove so plugins novos registados. Binarios anteriores, atualizados e modulos Adrenaline existentes sao mantidos. Configuracoes posteriores ao backup sao revertidas; before-uninstall preserva o estado encontrado. Originais ausentes podem ser recuperados sem sobrescrever um binario existente.

Cheats/mods dependem da ID e versao. Mods exigem ZIP de Release com SHA-256 e estrutura rePatch reconhecida; nao sobrescreve mods existentes. Fundos alternativos podem ser capas/icones. Atualizacoes/conteudo automatico para jogos PCS... e DLCs presentes nos catalogos locais. Rede, instalacao de conteudo/plugins e desinstalacao/reposicao fisica pendentes de validacao. Ver REVISAO_1.6.3_BETA.md.
