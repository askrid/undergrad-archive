import { PreloadedState } from "@reduxjs/toolkit";
import { render, RenderOptions } from "@testing-library/react";
import React, { PropsWithChildren } from "react";
import { Provider } from "react-redux";
import { MemoryRouter, Route, Routes } from "react-router-dom";
import { AppStore, RootState, setupStore } from "../store";

interface ExtendedRenderOptions extends Omit<RenderOptions, "queries"> {
  preloadedState?: PreloadedState<RootState>;
  store?: AppStore;
  path?: string;
  route?: string;
}

/**
 * Render a react element with provider that includes store and routers.
 */
export const renderWithProviders = (
  ui: React.ReactElement,
  { preloadedState = {}, path = "/", route = "/", ...renderOptions }: ExtendedRenderOptions = {}
) => {
  const store = setupStore(preloadedState);
  const Wrapper = ({ children }: PropsWithChildren<{}>) => (
    <Provider store={store}>
      <MemoryRouter initialEntries={[route]}>
        <Routes>
          <Route path={path} element={children} />
        </Routes>
      </MemoryRouter>
    </Provider>
  );

  return { store, ...render(ui, { wrapper: Wrapper, ...renderOptions }) };
};
