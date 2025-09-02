import Container from "../../components/Container";
import { renderWithProviders } from "../../test-utils";

describe("container", () => {
  it("should render", () => {
    const { container } = renderWithProviders(
      <Container>
        <div />
      </Container>
    );
    expect(container).toBeTruthy();
  });
});
