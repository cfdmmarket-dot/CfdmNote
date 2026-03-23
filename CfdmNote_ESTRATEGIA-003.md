● Sim, tudo está salvo na pasta do projeto /home/cfdm/Proj-CfdmNote_/src/. Aqui a estrutura dos arquivos fontes:                                                                              
                                                                                          
  src/                                                                                                                                                                                        
  ├── main.cpp                                                                                                                                                                                
  ├── gui/                                                                                                                                                                                    
  │   ├── AppLockDialog.h/.cpp          ← login + contador                                                                                                                                    
  │   ├── SecurityQuestionsDialog.h/.cpp ← NOVO: perguntas secretas                                                                                                                           
  │   ├── NoteEditorWidget.h/.cpp                                                                                                                                                             
  │   └── VaultDialog.h/.cpp                                                                                                                                                                  
  ├── notes/                                                                                                                                                                                  
  │   └── NoteManager.h/.cpp                                                                                                                                                                  
  └── db/                                                                                                                                                                                     
      └── MainDatabaseManager.h/.cpp                                                                                                                                                          
                                                                                                                                                                                              
  O que foi implementado na v1.0.5:                                                                                                                                                           
                                                                                                                                                                                              
  - Primeira vez → após criar a senha, abre tela para escolher 3 perguntas e respostas                                                                                                        
  - Login → contador mostra X/10 tentativas. Nos últimos 3, aviso em vermelho                                                                                                                 
  - 10 erros → apaga config + banco + encerra o app                                                                                                                                           
  - Esqueceu a senha? → aparece no login se perguntas foram configuradas. Acertou as 3 → define nova senha na mesma tela  
