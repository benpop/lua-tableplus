require "table"
require "_tableplus"


-- Iterate through map, in order, sorted by keys
function ordpairs (map)
  local keys, i = {}, 0
  for k in pairs(map) do
    keys[#keys+1] = k
  end
  table.sort(keys)
  return function ()
    i = i + 1
    local k = keys[i]
    if k ~= nil then return k, map[k] end
  end
end


function foreach (t,f)
  return table.foreach(t, f or print)
end


function foreachi (t,f)
  return table.foreachi(t, f or print)
end


function table.sorted (t,compare)
  local tt = table.copy(t)
  table.sort(tt,compare)
  return tt
end
