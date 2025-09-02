import { useSelector } from "react-redux";
import { useNavigate } from "react-router-dom";
import { selectArticle } from "../../store/slices/article";

const GoToArticleEditButton = () => {
  const { selectedArticle } = useSelector(selectArticle);
  const navigate = useNavigate();

  const handleClick = () => {
    navigate(`/articles/${selectedArticle?.id}/edit`);
  };

  return (
    <button
      id="edit-article-button"
      onClick={() => handleClick()}
      className="font-semibold px-4 py-2 rounded bg-white shadow-lg"
    >
      Edit
    </button>
  );
};

export default GoToArticleEditButton;
