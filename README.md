# NeuroTriagem AI — BitDogLab

Sistema de **pré-triagem de sinais de AVC (Acidente Vascular Cerebral)** desenvolvido sobre a placa educacional **BitDogLab** (baseada no Raspberry Pi Pico / RP2040). O projeto explora os periféricos embarcados da placa — joystick analógico, display OLED, matriz de LEDs, botões, buzzer e microfone — para simular testes rápidos de triagem inspirados no protocolo **F.A.S.T.** (Face, Arms, Speech, Time), amplamente usado no reconhecimento precoce de sintomas de AVC.

> ⚠️ **Aviso importante:** este é um projeto educacional/experimental (ex.: disciplina, curso ou hackathon envolvendo eletrônica embarcada). Ele **não é um dispositivo médico**, não realiza diagnóstico e não deve ser usado como substituto de avaliação profissional. Em caso de suspeita real de AVC, procure atendimento médico de emergência imediatamente.

## 🧠 Sobre o projeto

O protocolo F.A.S.T. orienta a identificação rápida de sinais de AVC observando:
- **F**ace (assimetria facial / queda de um dos lados do rosto)
- **A**rms (fraqueza ou dificuldade em manter os braços erguidos)
- **S**peech (fala arrastada ou incompreensível)
- **T**ime (tempo é crítico — agir rapidamente)

A proposta do **NeuroTriagem AI** é usar os sensores e atuadores da BitDogLab para criar testes interativos simples que emulam parte dessa triagem de forma lúdica e didática, servindo como prova de conceito de como hardware embarcado de baixo custo pode apoiar iniciativas de conscientização e triagem preliminar.

> 🚧 **Em constante evolução:** este repositório ainda receberá novos módulos de triagem além dos já disponíveis, expandindo a cobertura de sinais avaliados pelo protocolo F.A.S.T.

## 📁 Estrutura do repositório

```
NeuroTriagem_AI_BitDogLab/
├── teste_avc_joystick/     # Teste de coordenação/força motora usando o joystick analógico
└── teste_sorriso_nuvem/    # Teste relacionado à simetria/expressão facial ("sorriso")
```

- **`teste_avc_joystick/`** — módulo que utiliza o joystick analógico da BitDogLab para avaliar coordenação motora, estabilidade e tempo de resposta, sinais associados à fraqueza motora (componente "Arms" do F.A.S.T.).
- **`teste_sorriso_nuvem/`** — módulo voltado à avaliação de simetria/expressão facial ("sorriso"), servindo de apoio visual/interativo ao componente "Face" do F.A.S.T.

> 📌 Estrutura descrita a partir das pastas presentes no repositório. Ajuste esta seção com detalhes específicos de cada módulo (arquivos principais, fluxo de execução, etc.) conforme o projeto evoluir.

## 🔧 Hardware utilizado

Projeto desenvolvido para a placa **BitDogLab**, que integra em um único módulo (baseado no Raspberry Pi Pico/RP2040 ou RP2350):

- LED RGB
- Matriz de LEDs 5x5 (WS2812B)
- 3 botões
- Joystick analógico
- Buzzer(s)
- Microfone analógico
- Display OLED 128x64 (I2C)
- Conectores de expansão (jacaré, IDC, jumpers)

## 🚀 Como usar

1. Clone o repositório:
   ```bash
   git clone https://github.com/TheogenesGabriel/NeuroTriagem_AI_BitDogLab.git
   cd NeuroTriagem_AI_BitDogLab
   ```
2. Conecte a placa BitDogLab ao computador via USB.
3. Carregue o firmware/script desejado (`teste_avc_joystick` ou `teste_sorriso_nuvem`) na placa, utilizando o ambiente de desenvolvimento compatível (ex.: Thonny/MicroPython ou Pico SDK/C, conforme a linguagem do módulo).
4. Siga as instruções exibidas no display OLED da placa para executar o teste.

> 📌 Complete esta seção com os passos exatos de upload/execução (IDE utilizada, dependências, comandos específicos) de acordo com a linguagem em que cada módulo foi implementado.

## 📶 Conexão Wi-Fi

Para os módulos que dependem de conectividade (ex.: comunicação entre a BitDogLab/RP2040 e o computador), observe:

- A rede Wi-Fi deve ser **2,4 GHz** (o RP2040 não é compatível com redes 5 GHz).
- O RP2040 e o computador devem estar conectados **à mesma sub-rede**.
- O sistema tende a ficar **mais estável quando utilizado via dados móveis (hotspot do celular)** do que em redes Wi-Fi domésticas/corporativas, que costumam ter restrições de isolamento entre dispositivos.

## 🛠️ Tecnologias
- Placa **BitDogLab** (Raspberry Pi Pico / RP2040)
- MicroPython e/ou C (Pico SDK) — *ajustar conforme a linguagem usada em cada módulo*
