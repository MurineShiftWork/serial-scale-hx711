import importlib.metadata


def test_version_is_non_empty_string() -> None:
    version = importlib.metadata.version("serial-scale-hx711")
    assert isinstance(version, str)
    assert version != ""
