# Máquina de Turing Reversível

## Modo de Uso
O simulador pode ser executado da linha de comando com 3 flags:

- p: Imprime o estado das fitas para stdout a cada passo da máquina
- g: Visualização gráfica
- s: Lê a entrada de stdin ao invés de um arquivo

Caso s não esteja ativo, o simulador vai ler a entrada do arquivo passado como o último argumento.

Caso a string de entrada seja aceita pela máquina, o programa retornará o código de saida 0. Caso
seja rejeitada ou se algum erro ocorre durante a leitura ou execução, retornara um código de saída > 0

Exemplo de uso:

`./simulador -gs < entrada-quintupla.txt && echo Aceitado || echo Rejeitado`



## Padronização de Entrada e Saída

```
                        ┌──────┐                         
                        │  S   │                         
                   ┌────└──────┘────┐           ┌───────┐
                   │     ┌────┐     │           │  **<  │
┌────┐             │     │    │     │           │       │
│    │  B@>        │┌──┐ │    │ ┌──┐│    **<  ┌─┼──┐    │
│ A0 ┼────────────►││S0├─►    ├─►Sf├┼────────►│ A1 │◄───┘
│    │             │└──┘ │    │ └─┬┘│         └─┬──┘     
└────┘             │     │    │   │ │           │        
                   │     └────┘   │ │           │ @BS    
                   └──────────────┼─┘           │        
                                  │@BS          │        
                                  │       ┌─────▼────┐   
                                  │       │┌────────┐│   
                                  └──────►││   A2   ││   
                                          │└────────┘│   
                                          └──────────┘   
```

Dada uma máquina de turing padrão S, o artigo de Bennett diz que sua entrada e saída são padrões se:

* O resto da fita está vazio
* Não há espaços em brancos no meio da entrada/saída
* ***A cabeça da fita está na no espaço em branco imediatatemente a esquerda da entrada/saída***

Esse último ponto é importante, pois, após o final do primeiro estágio da Máquina de Turing Reversível de 3 estágios, assume-se que a saída é padrão, o que é crucial para que segundo estágio, onde a sáida é copiada para a terceira fita, rode corretamente.

Portanto, precisamos padronizar essa saída. Isso é feito com a adição de 3 estados, um símbolo especial (representado no diagrama como @, porém implementado no simulador como o byte 0xff) e 2z + 3 novas transições, onde z é o número de símbolos no alfabeto de fita. O primeiro estado é dado como o estado inicial da máquina. Esse estado simplesmente marca o espaço em branco onde ele inicia (notando que a entrada recebida pela definição da máquina é copiada a partir do segundo espaço da fita) como @, move o cabeçalho para a direita e procede com a execução da máquina S. Quando S chega em seu estado final, caso o cabeçalho tenha retornado ao primeiro espaço, esse espaço é esvaziado e a máquina procede para o novo estado final. Caso contrário, ela mantém o símbolo atual na fita e segue para a esquerda, repetindo até atingir @ e então prossegue para o espaço final.

## Modo Gráfico
O modo gráfico depende da biblioteca [tigr](https://github.com/erkkah/tigr/tree/master) para visualização.
