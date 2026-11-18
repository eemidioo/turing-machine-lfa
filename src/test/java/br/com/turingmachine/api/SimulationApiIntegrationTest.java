package br.com.turingmachine.api;

import com.fasterxml.jackson.databind.JsonNode;
import com.fasterxml.jackson.databind.ObjectMapper;
import org.junit.jupiter.api.Test;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.boot.test.autoconfigure.web.servlet.AutoConfigureMockMvc;
import org.springframework.boot.test.context.SpringBootTest;
import org.springframework.http.MediaType;
import org.springframework.test.web.servlet.MockMvc;

import static org.assertj.core.api.Assertions.assertThat;
import static org.hamcrest.Matchers.containsString;
import static org.springframework.test.web.servlet.request.MockMvcRequestBuilders.delete;
import static org.springframework.test.web.servlet.request.MockMvcRequestBuilders.get;
import static org.springframework.test.web.servlet.request.MockMvcRequestBuilders.post;
import static org.springframework.test.web.servlet.result.MockMvcResultMatchers.header;
import static org.springframework.test.web.servlet.result.MockMvcResultMatchers.jsonPath;
import static org.springframework.test.web.servlet.result.MockMvcResultMatchers.status;

@SpringBootTest
@AutoConfigureMockMvc
class SimulationApiIntegrationTest {

    @Autowired
    private MockMvc mockMvc;

    @Autowired
    private ObjectMapper objectMapper;

    @Test
    void createsStepsUntilAcceptanceAndDeletes() throws Exception {
        JsonNode created = objectMapper.readTree(mockMvc.perform(post("/api/simulations")
                        .contentType(MediaType.APPLICATION_JSON)
                        .content("{\"input\":\"aabbcc\"}"))
                .andExpect(status().isCreated())
                .andExpect(header().exists("Location"))
                .andExpect(jsonPath("$.status").value("READY"))
                .andExpect(jsonPath("$.state").value("q0"))
                .andExpect(jsonPath("$.headPosition").value(1))
                .andExpect(jsonPath("$.tape[0]").value("_"))
                .andReturn().getResponse().getContentAsString());

        String id = created.get("id").asText();

        mockMvc.perform(post("/api/simulations/{id}/step", id))
                .andExpect(status().isOk())
                .andExpect(jsonPath("$.stepCount").value(1))
                .andExpect(jsonPath("$.state").value("qFindB"))
                .andExpect(jsonPath("$.lastTransition.write").value("X"));

        String result;
        int steps = 1;
        do {
            result = mockMvc.perform(post("/api/simulations/{id}/step", id))
                    .andExpect(status().isOk())
                    .andReturn().getResponse().getContentAsString();
            steps++;
        } while (!objectMapper.readTree(result).get("status").asText().equals("ACCEPTED") && steps < 200);

        JsonNode accepted = objectMapper.readTree(result);
        assertThatStatus(accepted, "ACCEPTED");
        assertThat(accepted.get("accepted").asBoolean()).isTrue();
        assertThat(accepted.get("tape").toString()).contains("X", "Y", "Z");

        mockMvc.perform(delete("/api/simulations/{id}", id))
                .andExpect(status().isNoContent());

        mockMvc.perform(get("/api/simulations/{id}", id))
                .andExpect(status().isNotFound());
    }

    @Test
    void returnsProblemDetailForInvalidSymbol() throws Exception {
        mockMvc.perform(post("/api/simulations")
                        .contentType(MediaType.APPLICATION_JSON)
                        .content("{\"input\":\"abd\"}"))
                .andExpect(status().isBadRequest())
                .andExpect(jsonPath("$.title").value("Entrada inválida"))
                .andExpect(jsonPath("$.detail", containsString("alfabeto")));
    }

    @Test
    void returnsProblemDetailForBlankInput() throws Exception {
        mockMvc.perform(post("/api/simulations")
                        .contentType(MediaType.APPLICATION_JSON)
                        .content("{\"input\":\"\"}"))
                .andExpect(status().isBadRequest())
                .andExpect(jsonPath("$.title").value("Validação falhou"));
    }

    @Test
    void returnsNotFoundForUnknownId() throws Exception {
        mockMvc.perform(post("/api/simulations/nao-existe/step"))
                .andExpect(status().isNotFound())
                .andExpect(jsonPath("$.title").value("Simulação não encontrada"));
    }

    private static void assertThatStatus(JsonNode snapshot, String expected) {
        assertThat(snapshot.get("status").asText()).isEqualTo(expected);
    }
}
