import { render } from "@testing-library/react";
import { Provider } from "react-redux";
import App from "../App";
import { setupStore } from "../store";

describe("<App />", () => {
  it("should render", () => {
    render(
      <Provider store={setupStore()}>
        <App />
      </Provider>
    );
  });
});
