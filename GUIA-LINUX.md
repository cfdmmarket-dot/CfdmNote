# Como usar o CfdmNote no Linux
### Guia simples — sem complicação nenhuma

---

## O que você precisa

Só uma coisa: o arquivo **CfdmNote-2.2.0-linux-x86_64.AppImage**

Pense nele como um aplicativo numa caixinha. Tudo que precisa já está dentro dessa caixinha. Não precisa instalar nada.

---

## Passo 1 — Coloque o arquivo em algum lugar fácil

Mova o arquivo para uma pasta que você vai lembrar.
Por exemplo, a pasta **Downloads** ou crie uma pasta chamada **Aplicativos** na sua Área de Trabalho.

> 💡 Dica: pense no arquivo como um ícone de programa. Quanto mais fácil de achar, melhor.

---

## Passo 2 — Dê permissão para o arquivo abrir

O Linux precisa de uma "autorização" antes de rodar qualquer programa novo.
É como dar uma chave para a porta.

**Opção A — pelo mouse (mais fácil):**

1. Clique com o **botão direito** no arquivo
2. Clique em **Propriedades**
3. Procure a aba **Permissões**
4. Marque a caixinha que diz **"Permitir execução como programa"**
5. Feche a janela

**Opção B — pelo terminal (mais rápido):**

Abra o terminal, vá até a pasta onde está o arquivo e digite:

```
chmod +x CfdmNote-2.2.0-linux-x86_64.AppImage
```

Aperte **Enter**.

> 💡 Isso só precisa ser feito **uma vez**. Da próxima vez é só clicar.

---

## Passo 3 — Abra o aplicativo

**Dê dois cliques** no arquivo.

Pronto. O CfdmNote vai abrir.

---

## Na primeira vez que abrir

Vai aparecer uma tela pedindo uma **senha**.

- Se é a **primeira vez**, crie uma senha que você vai lembrar
- Essa senha protege os seus dados dentro do app
- Se errar a senha **10 vezes**, o app apaga os dados por segurança — então anote em algum lugar seguro

---

## O que fazer se não abrir com dois cliques

Alguns sistemas Linux perguntam o que fazer com o arquivo.
Se aparecer uma janela perguntando, escolha **"Executar"** ou **"Run"**.

Se ainda não abrir, abra o terminal, vá até a pasta do arquivo e digite:

```
./CfdmNote-2.2.0-linux-x86_64.AppImage
```

Aperte **Enter**.

---

## Posso colocar no pendrive?

**Sim!** Copie o arquivo para o pendrive normalmente.
Em qualquer computador Linux, repita os passos 2 e 3.
O app vai funcionar igual — sem instalar nada.

---

## Como fechar o app

Clique no **X** da janela, igual a qualquer outro programa.
Os seus dados são salvos automaticamente.

---

## Perguntas frequentes

**O arquivo é grande (41 MB), é normal?**
Sim. Tudo que o programa precisa está dentro dele — por isso é maior do que parece.

**Precisa de internet para funcionar?**
Não. O CfdmNote funciona completamente sem internet.

**Posso ter mais de um banco de dados?**
Sim. Você pode criar cofres criptografados separados pelo menu **CfdmNote → Criar Cofre**.

**Como faço backup dos meus dados?**
Use o menu **CfdmNote → Backup Externo** para salvar numa mídia removível.

---

## Precisa de ajuda?

Abra o manual dentro do próprio app:
- Pressione a tecla **F1**, ou
- Clique em **Ajuda** no menu superior

---

*CfdmNote v2.2.0 — feito para ser simples de usar*
