package br.com.turingmachine.api;

import br.com.turingmachine.api.ApiDtos.CreateSimulationRequest;
import br.com.turingmachine.api.ApiDtos.SimulationResponse;
import br.com.turingmachine.application.SimulationService;
import br.com.turingmachine.application.SimulationService.SimulationSession;
import jakarta.validation.Valid;
import org.springframework.http.ResponseEntity;
import org.springframework.web.bind.annotation.DeleteMapping;
import org.springframework.web.bind.annotation.GetMapping;
import org.springframework.web.bind.annotation.PathVariable;
import org.springframework.web.bind.annotation.PostMapping;
import org.springframework.web.bind.annotation.RequestBody;
import org.springframework.web.bind.annotation.RequestMapping;
import org.springframework.web.bind.annotation.RestController;
import org.springframework.web.servlet.support.ServletUriComponentsBuilder;

import java.net.URI;

/** Endpoints consumidos pela interface web. */
@RestController
@RequestMapping("/api/simulations")
public class SimulationController {

    private final SimulationService service;

    public SimulationController(SimulationService service) {
        this.service = service;
    }

    @PostMapping
    public ResponseEntity<SimulationResponse> create(@Valid @RequestBody CreateSimulationRequest request) {
        SimulationSession session = service.create(request.input());
        URI location = ServletUriComponentsBuilder.fromCurrentRequest()
                .path("/{id}").buildAndExpand(session.id()).toUri();
        return ResponseEntity.created(location).body(toResponse(session));
    }

    @GetMapping("/{id}")
    public SimulationResponse get(@PathVariable String id) {
        return toResponse(service.get(id));
    }

    @PostMapping("/{id}/step")
    public SimulationResponse step(@PathVariable String id) {
        SimulationSession session = service.get(id);
        return SimulationResponse.from(id, session.input(), session.step());
    }

    @DeleteMapping("/{id}")
    public ResponseEntity<Void> delete(@PathVariable String id) {
        service.delete(id);
        return ResponseEntity.noContent().build();
    }

    private SimulationResponse toResponse(SimulationSession session) {
        return SimulationResponse.from(session.id(), session.input(), session.current());
    }
}
