● FUSE não disponível no ambiente. Vou usar o modo de extração:                                                                                                                               
                                                                                                                                                                                              
● Bash(\                                                                                                                                                                                      
      VERSION=$(grep 'CFDMNOTE_VERSION' src/CfdmVersion.h | grep -oP '"\K[^"]+') && \…)                                                                                                       
  ⎿  appimagetool, continuous build (commit 5735cc5), build <local dev build> built on 2023-03-08 22:52:04 UTC                                                                                
     /home/cfdm/Proj-CfdmNote_/AppDir/cfdmnote.desktop: hint: value item "Security" in key "Categories" in group "Desktop Entry" can be extended with another category among the followin     
     g categories: Settings, or System                                                                                                                                                        
     … +43 lines (ctrl+o to expand)                                                                                                                                                           
                                                                                                                                                                                            
  Read 1 file (ctrl+o to expand)                                                                                                                                                              
                                                                                                                                                                                              
● Bash(git add package/ make-appimage.sh && git commit -m "$(cat <<'EOF'                                                                                                                      
      feat(v2.2.0): distribuição AppImage — CfdmNote-2.2.0-x86_64.AppImage…)                                                                                                                  
  ⎿  [main 8b3b48a] feat(v2.2.0): distribuição AppImage — CfdmNote-2.2.0-x86_64.AppImage                                                                                                      
      3 files changed, 157 insertions(+)                                                                                                                                                      
      create mode 100755 make-appimage.sh                                                                                                                                                     
     … +2 lines (ctrl+o to expand)                                                                                                                                                            
                                                                                                                                                                                              
● AppImage gerado: CfdmNote-2.2.0-x86_64.AppImage (41 MB)                                                                                                                                     
                                                                                                                                                                                              
  Para usar em qualquer Linux x86_64:                                                                                                                                                         
                                                                                                                                                                                              
  chmod +x CfdmNote-2.2.0-x86_64.AppImage                                                                                                                                                     
  ./CfdmNote-2.2.0-x86_64.AppImage                                                                                                                                                            
                                                                                                                                                                                              
  O arquivo está em /home/cfdm/Proj-CfdmNote_/. Inclui tudo: Qt5, Botan, plugins gráficos, traduções, ícones — sem precisar instalar nada.                                                    
                                                            
  Para gerar novamente no futuro (após qualquer atualização):                                                                                                                                 
  cmake -B build-release -DCMAKE_BUILD_TYPE=Release         
  cmake --build build-release -j$(nproc)                                                                                                                                                      
  bash make-appimage.sh  
