import { useDispatch } from "react-redux";
import { AppDispatch } from "../../store";
import { deleteComment } from "../../store/slices/comment";

interface Props {
  commentId: number;
}

const CommentDeleteButton = ({ commentId }: Props) => {
  const dispatch = useDispatch<AppDispatch>();

  const handleClick = () => {
    dispatch(deleteComment(commentId));
  };

  return (
    <button
      id="delete-comment-button"
      onClick={() => handleClick()}
      className="font-medium text-sm text-white px-2 bg-red-700 rounded shadow-inner"
    >
      Delete
    </button>
  );
};

export default CommentDeleteButton;
