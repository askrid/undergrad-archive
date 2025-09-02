import { Link } from "react-router-dom";
import LogoutButton from "./buttons/LogoutButton";
import Container from "./Container";

const Header = () => {
  return (
    <header className="sticky top-0 bg-white shadow-md bg-opacity-60 backdrop-blur z-50">
      <Container>
        <nav className="flex justify-between items-center py-3">
          <ul>
            <li>
              <Link to="/articles" className="font-extrabold italic text-3xl">
                YASB
              </Link>
            </li>
          </ul>
          <ul>
            <li>
              <LogoutButton />
            </li>
          </ul>
        </nav>
      </Container>
    </header>
  );
};

export default Header;
