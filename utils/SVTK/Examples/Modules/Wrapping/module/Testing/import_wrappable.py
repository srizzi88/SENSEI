from wrapping import svtkWrappable

wrap = svtkWrappable.svtkWrappable()
assert wrap.GetString() == 'wrapped'
