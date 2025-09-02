import { useDispatch } from "react-redux";
import { AppDispatch } from "../../store";
import { updateComment } from "../../store/slices/comment";

interface Props {
  commentId: number;
  initialContent: string;
}

const CommentEditButton = ({ commentId, initialContent }: Props) => {
  const dispatch = useDispatch<AppDispatch>();

  const handleClick = () => {
    const content = prompt("Edit comment", initialContent);

    // Content is neither null nor a empty string.
    if (content) {
      dispatch(updateComment({ id: commentId, content }));
    }
  };

  return (
    <button
      id="edit-comment-button"
      onClick={() => handleClick()}
      className="font-medium text-sm text-white px-2 rounded bg-slate-700 shadow-inner"
    >
      Edit
    </button>
  );
};

export default CommentEditButton;
