import { useDispatch, useSelector } from "react-redux";
import { useNavigate } from "react-router-dom";
import { AppDispatch } from "../../store";
import { deleteArticle, selectArticle } from "../../store/slices/article";

const ArticleDeleteButton = () => {
  const { selectedArticle } = useSelector(selectArticle);
  const dispatch = useDispatch<AppDispatch>();
  const navigate = useNavigate();

  const handleClick = () => {
    dispatch(deleteArticle(selectedArticle?.id!));
    navigate("/articles");
  };

  return (
    <button
      id="delete-article-button"
      onClick={() => handleClick()}
      className="font-semibold text-white px-4 py-2 rounded bg-red-800 shadow-lg"
    >
      Delete
    </button>
  );
};

export default ArticleDeleteButton;
