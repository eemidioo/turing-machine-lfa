# Máquina de Turing — Proposta em Python

Deixei essa versão aqui em uma branch separada como uma proposta para o nosso trabalho de Linguagens Formais e Autômatos. Montei com base no que venho estudando da disciplina, buscando atender de forma direta aos critérios pedidos no projeto (execução passo a passo na fita, estados, transições, definição formal e testes).

A ideia aqui foi deixar tudo bem simples, didático e fácil de entender na hora da apresentação. Como é só uma proposta, está totalmente aberta para a gente modificar, melhorar ou aproveitar o que fizer sentido para o grupo.

## O que ela faz
- **Foco principal:** permite testar qualquer Máquina de Turing (já deixei prontos na pasta `exemplos/` os arquivos de **Palíndromos em `{a, b}*`** e da linguagem **`a^n b^n c^n` com `n >= 1`**).
- **Execução passo a passo:** mostra a fita andando, o cabeçote lendo/escrevendo/movendo, um log de cada movimento e o rastro completo da fita.
- **Diagrama de estados:** desenha o grafo automaticamente a partir das regras (dá para arrastar os estados, dar zoom e baixar em `.svg` para colocar nos slides/PDF).
- **Parte formal:** gera automaticamente a 7-tupla e a tabela de transições.
- **Edição livre:** dá para importar/baixar arquivos `.txt`, editar as regras em texto ou montar direto na tabela.

## Como executar
1. Instale a única dependência do projeto (Streamlit):
   pip install -r requirements.txt
2. Rode a aplicação:
python -m streamlit run app.py
3. Quando abrir no navegador, clique em Importar ou baixar arquivo da máquina (.txt), carregue um dos arquivos da pasta exemplos/ (ou escreva suas próprias regras), digite uma entrada na fita e clique em Executar.
