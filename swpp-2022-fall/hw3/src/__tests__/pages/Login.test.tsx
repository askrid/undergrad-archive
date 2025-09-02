import { fireEvent, screen, waitFor } from "@testing-library/react";
import Login from "../../pages/Login";
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

describe("login page", () => {
  beforeEach(() => {
    jest.clearAllMocks();
  });

  it("should render", () => {
    const { container } = renderWithProviders(<Login />);
    expect(container).toBeTruthy();
  });

  describe("login form", () => {
    it("should have email input, password input, and login button", () => {
      renderWithProviders(<Login />);
      expect(screen.getByPlaceholderText(/email/i)).toBeInTheDocument();
      expect(screen.getByPlaceholderText(/password/i)).toBeInTheDocument();
      expect(screen.getByRole("button", { name: /Log In/i })).toBeInTheDocument();
    });

    it("might accept login without alert", async () => {
      mockUnwrapResult.mockReturnValue(true);
      jest.spyOn(window, "alert").mockImplementation(() => {});
      renderWithProviders(<Login />);
      fireEvent.change(screen.getByPlaceholderText(/email/i), { target: { value: "test@example.com" } });
      fireEvent.change(screen.getByPlaceholderText(/password/i), { target: { value: "password" } });
      fireEvent.click(screen.getByRole("button", { name: /Log In/i }));
      await waitFor(() => {
        expect(window.alert).not.toBeCalled();
      });
    });

    it("might reject login with alert", async () => {
      mockUnwrapResult.mockReturnValue(false);
      jest.spyOn(window, "alert").mockImplementation(() => {});
      renderWithProviders(<Login />);
      fireEvent.change(screen.getByPlaceholderText(/email/i), { target: { value: "test@example.com" } });
      fireEvent.change(screen.getByPlaceholderText(/password/i), { target: { value: "password" } });
      fireEvent.click(screen.getByRole("button", { name: /Log In/i }));
      await waitFor(() => {
        expect(window.alert).toBeCalled();
      });
    });
  });
});
