class Expr
  def put_logical_op(sis, i, op, start)
    j = i
    nparens, ndifops, r = 1, 0, 0
    while (j -= 1) >= 0
      s_ = sis[j]
      if s_.flags & SCAN_POP != 0
        ndifops += 1
        nparens += 1
      elsif s_.flags & SCAN_PUSH != 0
        nparens -= 1
        if nparens == 0
          if r == 0
            if ndifops != 0
              if j != 0 && op != GRN_OP_AND_NOT
                nparens = 1
                ndifops = 0
                r = j
              else
                s_ = alloc_si start
                if !s_
                  free sis, i
                  return nil
                else
                  s_.flags = SCAN_POP
                  s_.logical_op = op
                  sis[i] = s_
                  i += 1
                  return i
                end
              end
            else
              s_.flags &= ~SCAN_PUSH
              s_.logical_op = op
              return i
            end
          elsif ndifops != 0
            s_ = alloc_si start
            if !s_
              free sis, i
              return nil
            else
              s_.flags = SCAN_POP
              s_.logical_op = op
              sis[i] = s_
              i += 1
              return i
            end
          else
            s_.flags &= ~SCAN_PUSH
            s_.logical_op = op
            k = i
            (j...r).each do |l|
              sis[k] = sis[l]
              k += 1
            end
            k = j
            (r...i).each do |l|
              sis[k] = sis[l]
              k += 1
            end
            l = i
            ((i + j - r)...i).each do |k|
              sis[k] = sis[l]
              l += 1
            end
            return i
          end
        end
      elsif op == GRN_OP_AND_NOT || op != s_.logical_op
        ndifops += 1
      end
    end
    err GRN_INVALID_ARGUMENT, "unmatched nesting level"
    free sis, i
    return nil
  end
  def free(sis, i)
    i.times do |j|
      si = sis[j]
      si.free
    end
    sis.free
  end
  def build(sis, var, op, size)
    i = 0
    stat = SCAN_START
    si = nil
    each do |c|
      case c.op
      when GRN_OP_MATCH, GRN_OP_NEAR, GRN_OP_NEAR2,
           GRN_OP_SIMILAR , GRN_OP_PREFIX, GRN_OP_SUFFIX,
           GRN_OP_EQUAL, GRN_OP_NOT_EQUAL, GRN_OP_LESS,
           GRN_OP_GREATER, GRN_OP_LESS_EQUAL,
           GRN_OP_GREATER_EQUAL, GRN_OP_GEO_WITHINP5,
           GRN_OP_GEO_WITHINP6, GRN_OP_GEO_WITHINP8,
           GRN_OP_TERM_EXTRACT
        stat = SCAN_START
        si.op = c.op
        si.fin = index c
        sis[i] = si
        i += 1
        si.each_arg do |arg|
          if arg.type == GRN_EXPR
            e = arg.to_expr
            eci = 0
            while eci < e.codes_curr
              ec = e[eci]
              if ec.value
                case ec.value.type
                when GRN_ACCESSOR
                  index, sid = ec.value.column_index c.op
                  if index
                    weight = ec.weight
                    si.flags |= SCAN_ACCESSOR
                    if ec.value.to_accessor.next
                      si.put_index ec.value, sid, weight
                    else
                      si.put_index index, sid, weight
                    end
                  end
                when GRN_COLUMN_FIX_SIZE , GRN_COLUMN_VAR_SIZE
                  index, sid = ec.value.column_index c.op
                  si.put_index index, sid, ec.weight if index
                when GRN_COLUMN_INDEX
                  index, sid = ec.value, 0
                  if e[eci + 2] &&
                     e[eci + 1].value &&
                     e[eci + 1].value.domain == GRN_DB_UINT32 &&
                     e[eci + 2].op == GRN_OP_GET_MEMBER
                    sid = e[eci + 1].value.inspect.to_i + 1
                    eci += 2
                    ec = e[eci]
                  end
                  si.put_index index, sid, ec.weight if index
                end
              end
              eci += 1
            end
          elsif arg.db?
            index, sid = arg.column_index c.op
            si.put_index index, sid, 1 if index
          elsif arg.accessor?
            si.flags |= SCAN_ACCESSOR
            index, sid = arg.column_index c.op
            if index
              if arg.to_accessor.next
                si.put_index arg, sid, 1
              else
                si.put_index index, sid, 1
              end
            end
          else
            si.query = arg
          end
        end
        si = nil
      when GRN_OP_AND, GRN_OP_OR, GRN_OP_AND_NOT,
           GRN_OP_ADJUST
        i = put_logical_op sis, i, c.op, index(c)
        stat = SCAN_START
      when GRN_OP_PUSH
        si = alloc_si index(c) unless si
        if !si
          free sis, i
          i = nil
        elsif c.value == var
          stat = SCAN_VAR
        else
          si.push_arg c.value
          si.flags |= SCAN_PRE_CONST if stat == SCAN_START
          stat == SCAN_CONST
        end
      when GRN_OP_GET_VALUE
        si = alloc_si index(c) unless si
        case sis && stat
        when nil
          free sis, i
          i = nil
        when SCAN_START, SCAN_CONST, SCAN_VAR
          stat = SCAN_COL1
          si.push_arg c.value
        when SCAN_COL1
          err GRN_INVALID_ARGUMENT,
              "invalid expression: can't use column" +
              " as a value: <#{c.value.name}>: " +
              "<#{inspect}>"
          free sis, i
          i = nil
        when SCAN_COL2
        end
      when GRN_OP_CALL
        si = alloc_si index(c) unless si
        if !si
          free sis, i
          i = nil
        elsif c.flags(GRN_EXPR_CODE_RELATIONAL_EXPRESSION) ||
              index(c) + 1 == codes_curr
          stat = SCAN_START
          si.op = c.op
          si.fin = index(c)
          sis[i] = si
          i += 1
          si.each_arg do |arg|
            if arg.db?
              index, sid = arg.column_index c.op
              si.put_index index, sid, 1 if index
            elsif arg.accessor?
              si.flags |= SCAN_ACCESSOR
              index, sid = arg.column_index c.op
              si.put_index index, sid, 1 if index
            else
              si.query = arg
            end
          end
          si = nil
        else
          stat = SCAN_COL2
        end
      end
      i
    end
    return nil unless i
    if op == GRN_OP_OR && size == 0
      si = sis[0]
      if si.flags & SCAN_PUSH == 0 || si.logical_op != op
        err GRN_INVALID_ARGUMENT, 'invalid expr'
        free sis, i
        return nil
      else
        si.flags &= ~SCAN_PUSH
        si.logical_op = op
      end
      return i
    else
      return put_logical_op sis, i, op, codes_curr
    end
  end
end
