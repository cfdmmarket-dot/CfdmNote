1. Sobre as entradas (senhas/credenciais):                                                                                                                                                  
  - Se não vão para cofre, ficam no .ksdb sem criptografia?  Sim Nao Havera Cryptografia na Base depois teremos cofres encfriptados                                                                                                                                 
  - Ou haverá algum tipo de proteção (senha mestra, criptografia de campo)?                                                                                                                   
                                                                                                                                                                                              
  2. O que você quer herdar do KeePassXC: Vou Herdar tudo, menos o cofre, manter tudo parecido talvez mude poucas coisas                                                                                                                                                    
  - Só o layout visual (grupos + tabela de entradas)?                                                                                                                                         
  - O modelo de dados (Entry, Group)?                                                                                                                                                         
  - O Auto-Type (preencher senha automaticamente)?                                                                                                                                            
  - Ou só a ideia geral?                                                                                                                                                                      
                                                                                                                                                                                              
  3. Funcionalidades do Obsidian que quer incluir:                                                                                                                                            
  - [[wiki-links]] entre notas — já planejado                                                                                                                                                 
  - Grafo de conhecimento — já planejado                                                                                                                                                      
  - O que mais? (templates, tags, backlinks, canvas, plugins?)   tudo o que puder e aindfa tenho mais ideias a incorporar mas de inicio o basico pra funcionar                                                                                                                             
                                                                                                                                                                                              
  4. Relação entre entradas e notas:                                                                                                                                                          
  - Cada entrada pode ter uma nota vinculada?         pode ser vinculadas, ou comno achar melhor                                                                                                                                        
  - Ou entradas e notas são coisas separadas e independentes?                                                                                                                                 
                                                                                                                                                                                              
  5. Público/uso:                                                                                                                                                                             
  - Só para você (uso pessoal)?                                                                                                                                                               
  - Ou pretende distribuir para outros?                                                                                                                                                       
                                                                                                                                                                                              
  Com isso claro, ajusto o CMakeLists, a arquitetura e o que herdar (ou não) do KeePassXC.   
