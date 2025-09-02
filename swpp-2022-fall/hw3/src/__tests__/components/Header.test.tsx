import { fireEvent, screen, waitFor } from "@testing-library/react";
import Header from "../../components/Header";
import { renderWithProviders } from "../../test-utils";

const mockDispatch = jest.fn();
jest.mock("react-redux", () => ({
  ...jest.requireActual("react-redux"),
  useDispatch: () => mockDispatch,
}));

const mockUnwrapResult = jest.fn();
jest.mock("@reduxjs/toolkit", () => ({
  ...jest.requireActual("@reduxjs/toolkit"),
  unwrapResult: () => mockUnwrapResult(),
}));

describe("header", () => {
  beforeEach(() => {
    jest.clearAllMocks();
  });

  it("should render", () => {
    const { container } = renderWithProviders(<Header />);
    expect(container).toBeTruthy();
  });

  it("should have logo link", () => {
    renderWithProviders(<Header />);
    expect(screen.getByRole("link", { name: "YASB" })).toBeInTheDocument();
  });

  describe("logout button", () => {
    it("should handle log out", async () => {
      renderWithProviders(<Header />);
      fireEvent.click(screen.getByRole("button", { name: /Log out/i }));
      await waitFor(() => {
        expect(mockDispatch).toBeCalled();
      });
    });

    it("should handle log out error", async () => {
      renderWithProviders(<Header />);
      mockUnwrapResult.mockImplementation(() => {
        throw new Error();
      });
      jest.spyOn(console, "log").mockReturnValue();
      fireEvent.click(screen.getByRole("button", { name: /Log out/i }));
      await waitFor(() => {
        expect(console.log).toBeCalled();
      });
    });
  });
});
