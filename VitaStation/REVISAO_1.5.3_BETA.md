# VitaStation 1.5.3 beta — inicialização dos certificados

APP_VER 01.53, TITLE_ID VTST00001.

O registo real da consola com a versão 1.5.2 mostra «[https-ca] Unknown error», seguido de HTTPS 58 «error signaled by ssl ctx callback». Não mostra a importação das raízes. No caminho deste callback, std::call_once pode lançar std::system_error antes de chamar prepare_ca_store; as falhas de importação têm mensagens distintas.

Esta revisão substitui std::call_once por std::mutex com estado explícito. O cliente já utiliza std::mutex na inicialização de rede que chegou a funcionar nesta consola. Mantém as 118 raízes DER, a validação HTTPS e o carregamento PNG em memória. Acrescenta o código e categoria de eventuais erros de sincronização ao registo. A versão é identificada como VS1.5.3.

Instalar VitaStation-1.5.3-beta.vpk como atualização no VitaShell. Fechar a aplicação anterior, abrir novamente e entrar na loja. Quadrado permite repetir pedidos de carátulas. Não necessita de novos plugins.

Compilação Vita e integridade do VPK verificadas. O teste desktop libcurl/OpenSSL continua a importar as 118 raízes e a descarregar imagens com validação ativa. O comportamento desta alteração no dispositivo ainda requer confirmação; o teste no PC não reproduz o suporte de sincronização do VitaSDK.

Demos e regiões ASIA/Asian continuam desativadas na loja. Restantes funções e limites das revisões 1.5 a 1.5.2 mantidos. Nota do linker do SDK em crtn.o permanece.
