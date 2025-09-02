import axios from "axios";
import { AppStore, setupStore } from "../../store";
import { getAuth, login, logout, UserData } from "../../store/slices/auth";

const stubUserData: UserData = {
  id: 1,
  email: "test@example.com",
  password: "password",
  name: "john",
  logged_in: false,
};

describe("auth slice", () => {
  let store: AppStore;

  beforeEach(() => {
    store = setupStore();
    jest.clearAllMocks();
  });

  it("should get auth so that app can know whether the user is logged in", async () => {
    axios.get = jest.fn().mockResolvedValue({ data: { ...stubUserData, logged_in: true } });
    await store.dispatch(getAuth());
    expect(store.getState().auth.loginStatus).toEqual("LOGGED_IN");

    axios.get = jest.fn().mockResolvedValue({ data: stubUserData });
    await store.dispatch(getAuth());
    expect(store.getState().auth.loginStatus).toEqual("LOGGED_OUT");
  });

  it("should handle login success", async () => {
    axios.get = jest.fn().mockResolvedValue({ data: stubUserData });
    axios.patch = jest.fn();
    await store.dispatch(login({ email: stubUserData.email, password: stubUserData.password }));
    expect(store.getState().auth.loginStatus).toEqual("LOGGED_IN");
  });

  it("should handle login fail", async () => {
    axios.get = jest.fn().mockResolvedValue({ data: stubUserData });
    await store.dispatch(login({ email: stubUserData.email, password: stubUserData.password + "1" }));
    expect(store.getState().auth.loginStatus).toEqual("LOADING");
  });

  it("should handle logout after login", async () => {
    axios.get = jest.fn().mockResolvedValue({ data: stubUserData });
    axios.patch = jest.fn();
    await store.dispatch(login({ email: stubUserData.email, password: stubUserData.password }));
    await store.dispatch(logout());
    expect(store.getState().auth.loginStatus).toEqual("LOGGED_OUT");
  });
});
